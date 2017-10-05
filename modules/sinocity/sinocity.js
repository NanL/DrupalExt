// JavaScript Document for sinoCity
jQuery(function($){
	var city = $('.form-type-sinocity').eq(0).find('select.form-select.city').children('option');
	$('.form-type-sinocity > select.form-select.prov').change(function(){
		$(this).next('select').empty().append(city.filter('[value^=' + $(this).val() + ']').clone());
	}).change();
});
