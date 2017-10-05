<?php // Evernote Data Access and Management


// $EDAM = array();    // Evernote Data Access and Management
// $HOST = $GLOBALS['language']->language == 'zh-hans' ? 'app.yinxiang.com' : 'www.evernote.com';
// $HOST = in_array(substr($_SERVER['HTTP_HOST'],0,3), array('127','dev')) ? 'sandbox.evernote.com' : $HOST;
// $SIGN = str_replace(array('www.','app.','.com', '.evernote', '.evernote.com'), NULL, $HOST);

// define('EDAM_SERVICE_HOST', $HOST);
// define('EDAM_SERVICE_SIGN', $SIGN);
// define('EDAM_CONSUMER_KEY', 'nan');
// define('EDAM_CONSUMER_SECRET', '1234567890');

require_once(dirname(__FILE__).'/evernote/Evernote/Client.php');


// connect to Evernote
function pageUserLinkEvernote($user, $mark){

    // global $conf;
    // if(!in_array($mark, array('sandbox','evernote','yinxiang'))) drupal_not_found();

    if(variable_get('test')) $mark = 'sandbox';     // for testing

    $config = variable_get('apps')[$mark];

    $client = new Evernote\Client(array(
        'serviceHost' => $config['host'],
        'consumerKey' => $config['key'],
        'consumerSecret' => $config['secret'],
    ));

    if(empty($_GET['oauth_token']) || empty($_GET['oauth_verifier'])){

        $_SESSION['requestToken'] = $client->getRequestToken($GLOBALS['base_url'].'/'.$_GET['q']);
        drupal_goto($client->getAuthorizeUrl($_SESSION['requestToken']['oauth_token']));

    } else {

        $accessToken = $client->getAccessToken(
            $_GET['oauth_token'],
            $_SESSION['requestToken']['oauth_token_secret'],
            $_GET['oauth_verifier']
        );

        db_merge('note_auth')->key(array(
                'uid' => $user->uid,
                'type' => $mark,
            ))->fields(array(
                'born' => time(),
                'life' => intval($accessToken['edam_expires']/1000),
                'auth' => serialize($accessToken),
            ))->execute();

        drupal_set_message(t('Link to Evernote success !!'));

    }

    drupal_goto('user/'.$user->uid.'/link');

}


function evernote_cron_user($auth){

    if(!in_array($auth->type, array('sandbox','evernote','yinxiang'))) return;

    $auth->auth = unserialize($auth->auth);

    $book = cache_get($auth->type.':list:u'.$auth->uid, 'note_temp');

    if(empty($book)){

        $config = variable_get('apps')[$auth->type];

        $filter = new EDAM\NoteStore\NoteFilter(array('words'=>'any: tag:OPEN tag:公开'));
        $result = new EDAM\NoteStore\NotesMetadataResultSpec(array('includeUpdated'=>TRUE));
        // http://dev.evernote.com/doc/reference/NoteStore.html#Struct_NotesMetadataResultSpec

        $client = new Evernote\Client(array(
            'serviceHost' => $config['host'],
            'token' => $auth->auth['oauth_token'],
        ));

        $book = new stdClass();
        $book->cid  = $auth->type.':list:u'.$auth->uid;
        $book->data = $client->getNoteStore()->findNotesMetadata($filter, 0, 999, $result);

        cache_set($book->cid, $book->data, 'note_temp', time()+86400);    // day:86400s

    } else {

        foreach ($book->data->notes as $note) {

            db_merge('note_node')->key(array(
                'type' => $auth->type,
                'guid' => $note->guid,
            ))->fields(array(
                'uid'  => $auth->uid,
                'time' => intval($note->updated/1000),
            ))->execute();

        }

    }

    // print_r($book);

}


function evernote_cron_node($meta){

    if(!in_array($meta->type, array('sandbox','evernote','yinxiang'))) return;

    $auth = unserialize($meta->auth);
    $note = cache_get($meta->type.':note:'.$meta->guid, 'note_temp');

    if(empty($note)){

        $config = variable_get('apps')[$meta->type];
        $client = new Evernote\Client(array(
            'serviceHost' => $config['host'],
            'token' => $auth['oauth_token'],
        ));

        $note = new stdClass();
        $note->cid  = $meta->type.':note:'.$meta->guid;
        $note->data = $client->getNoteStore()->getNote($meta->guid, TRUE, TRUE, TRUE, TRUE);

        cache_set($note->cid, $note->data, 'note_temp');

    } else {

        $node = empty($meta->nid) ? new stdClass() : node_load($meta->nid);

        if(isset($node->changed) && $node->changed == $meta->time) return;

        if(empty($meta->nid))  $node->is_new = TRUE;
        if(empty($node->uid))  $node->uid  = $meta->uid;
        if(empty($node->type)) $node->type = 'note';
        if(empty($node->note)) $node->note = $meta;
        if(empty($node->edam)) $node->edam = $note->data;

        node_save($node);   // check evernote_node_presave  // new node not run presave !!!

        if(empty($meta->nid) && !empty($node->nid)){
            db_query('UPDATE {note_node} SET nid = (:nid) WHERE guid = (:guid) AND type = (:type)',
                array(':guid'=>$meta->guid, ':type'=>$meta->type, ':nid'=>$node->nid));
        }

    }

    // print_r($note);

}


