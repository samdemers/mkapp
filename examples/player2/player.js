function set_artist(artist) {
    $(".artist").text(artist);
}

function set_title(title) {
    $(".title").text(title);
}

function init() {

    $(".button").fadeTo("fast", 0.3);
    $(".button").mouseenter(function(event) { $(this).fadeTo("fast", 1); });
    $(".button").mouseleave(function(event) { $(this).fadeTo("fast", 0.3); });

    $(".button").mousedown(function(event) {
            $(this).addClass('button-clicked');
        });
    $(".button").mouseup(function(event) {
            $(this).removeClass('button-clicked');
        });

    $("#back").click(function(event)    { alert("back"); });
    $("#play").click(function(event)    { alert("play"); });
    $("#pause").click(function(event)   { alert("pause"); });
    $("#stop").click(function(event)    { alert("stop"); });
    $("#forward").click(function(event) { alert("forward"); });
    $("#open").click(function(event)    { alert("open"); });
}

$(document).ready(init);
