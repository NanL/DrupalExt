<?php // pageUser.inc


function pageUserHome($user, $html = ''){

    drupal_set_title($user->name);
    $html .= 'pageUser pageUserHome.inc';

    return $html;
}



function pageUserLink($user, $html = ''){

    $html .= 'pageUser pageUserLink.inc';


    return $html;
    // return user_view_page($account);
}
