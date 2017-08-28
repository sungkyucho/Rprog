/*
$('.collapse .nav li').click(function(){
	$('li').removeClass('clicked');
	$(this).addClass('clicked');
});
*/

$(document).ready(function(){
	setInterval(function(){cache_clear()},3000);
});
function cache_clear() {
/*	 window.location.reload(true); */
}
setInterval(function(){
	$(".blinkBGRed").toggleClass("backgroundRed");
	$(".blinkBGOrange").toggleClass("backgroundOrange");
},500)

