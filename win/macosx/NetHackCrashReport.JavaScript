#!/usr/bin/osascript -l JavaScript

// NetHack 3.7  tile.h       $NHDT-Date: 1693083762 2023/08/26 21:02:42 $  $NHDT-Branch: keni-crashweb2 $:$NHDT-Revision: 1.1 $
//      Copyright (c) 2023 Kenneth Lorber
// NetHack may be freely redistributed.  See license for details.

// Call with URL then field value pairs.  Opens a new browser window
// to: URL?field=value+field=value.....
// This program encodes the values; fieldnames don't require encoding.

// Should be installed in the playground.

function run(argv){

    var url = argv[0];
    var argcp = 1;

    url += "?cos=1";	    // Start the query string and set mode
    while(argcp < argv.length){
	url += "&" + argv[argcp] + "=" + encodeURIComponent(argv[argcp+1])
	argcp += 2;
    }

    var safari = Application('Safari');
    var nw = safari.make({ new:"document" });
    nw.url = url;
    safari.activate();
}
