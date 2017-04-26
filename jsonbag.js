/**
 JSON-bag Emanuele Ruffaldi 2017
 *
 * Uses code from github.com/manuelstofer/json-pointer for JSON Pointer patching
 */

// adjustes the XHR to support JSONbag
function jsonbag_prepare(xhr,multipart)
{
	xhr.responseType = "arraybuffer";
	if(multipart)
	{
		xhr.setRequestHeader("Accept","multipart/related")
	}
	else
	{
		xhr.setRequestHeader("Accept","application/jsonbag")
	}
}

// linkdata:image/png;offset=20345;size=20241
function jsonbag_linkdataparse(u)
{
	var a = u.split(";")
	var r = {}
	// TODO check linkdata: prefix
	r.mime = a[0].substr(9)
	for(var i = 1; i < a.length; i++)
	{
		var b = u.split("=");
		if(b.length == 2)
			r[b[0]] = b[1]
		else
			r[b[0]] = ""
	}
	return r
}

// debugging 
function jsonbag_uint8array2buffer(a)
{
  var view = new Uint8Array(a.length)

  for (var i = 0; i < a.length; i ++) {
	  view[i] = a[i]
  }

  return view.buffer	
}


// debugging 
function jsonbag_hex2buffer(hex)
{
  var view = new Uint8Array(hex.length / 2)

  for (var i = 0; i < hex.length; i += 2) {
    view[i / 2] = parseInt(hex.substring(i, i + 2), 16)
  }

  return view.buffer	
}

// supports JSON or JSON-bag in ArrayBuffer
function jsonbag_parse(xhr,fx)
{
	var r = xhr.response; 
    var joffset = 0;
    var jlength = r.byteLength;

	// "JBAG0000" + "HEXLENGTH"" + "\r\n""
	// maxlength is 8+2+8
	var r2 = new Uint16Array(r,0,18)
	if(r2[0] == 0x424a && r2[1] == 0x4741) // JBAG
	{
		if(r2[2] != 0x3030) // 0000
		{
			console.log("unknown version ",r2[2].toString(16))
			return
		}
		else
		{
			var sizehex = 0;
			for(var i = 3; i < 10 && r2[i] != 0xa0d; i++)
			{
				// r4[i] contains the 16bit of the two characters
				// quick and dirty HEX
				var j = r2[i]
				var lo = j >> 8;
				var hi = j & 0xFF;	
				lo = (lo > 57 ? lo-65+10:lo-48);
				hi = (hi > 57 ? hi-65+10:hi-48)*16;
				sizehex = (sizehex << 8 ) + lo + hi;
			}
			joffset = 2*(i+1);
			jlength = sizehex;
		}
	}

    if(xhr.getResponseHeader("Content-Type") == "application/jsonbag")
	{
		var h = xhr.getResponseHeader("JSONBag-Range")
	    if(h)
	    {
	    	var a = h.split(" ");
	    	var f = parseInt(a[0])
	    	var s = parseInt(a[1])
	    	joffset = f;
	    	jlength = s;
	    }
	}
    console.log("jsonbag_parse found offset/length: ",joffset,jlength)

    if(jlength > 0)
    {
		var dataView = new DataView(r,joffset,jlength); 				
		var decoder = new TextDecoder("utf-8");
		var decodedString = decoder.decode(dataView);	 
		var j = JSON.parse(decodedString)   	
	 	var c = joffset+jlength+3;

	 	var payloads = []

		while(c < r.byteLength-2)
		{
			var dataView = new DataView(r,c,2); 
			var cn = dataView.getInt16(0,true);
			var dataView = new DataView(r,c+2,cn);
			var decoder = new TextDecoder("utf-8");
			var decodedString = decoder.decode(dataView);	 
			var bn = JSON.parse(decodedString)   

			c += cn+2

			// cn can be used also for alignmentì
			var blob = new Blob([new Int8Array(r,c,bn.size)], {type : bn.mime});
			var url = URL.createObjectURL(blob);
			c += bn.size
			bn.blob = blob
			bn.url = url
			if(bn.path)
			{
				jsonpointer_set(j,bn.path,url)
			}
			payloads.push(bn)
		}
		fx(j,payloads)
	}
	else
	{
		testfx()	
	}
}

jsonpointer_unescape = function unescape (str) {
    return str.replace(/~1/g, '/').replace(/~0/g, '~');
};

jsonpointer_parse = function parse (pointer) {
    if (pointer === '') { return []; }
    if (pointer.charAt(0) !== '/') { throw new Error('Invalid JSON pointer: ' + pointer); }
    return pointer.substring(1).split(/\//).map(jsonpointer_unescape);
};

// https://github.com/manuelstofer/json-pointer/blob/master/index.js
function jsonpointer_set(obj,pointer,value)
{
	var refTokens = Array.isArray(pointer) ? pointer : jsonpointer_parse(pointer),
	  nextTok = refTokens[0];

    if (refTokens.length === 0) {
      throw Error('Can not set the root object');
    }

    for (var i = 0; i < refTokens.length - 1; ++i) {
        var tok = refTokens[i];
        if (tok === '-' && Array.isArray(obj)) {
          tok = obj.length;
        }
        nextTok = refTokens[i + 1];

        if (!(tok in obj)) {
            if (nextTok.match(/^(\d+|-)$/)) {
                obj[tok] = [];
            } else {
                obj[tok] = {};
            }
        }
        obj = obj[tok];
    }
    if (nextTok === '-' && Array.isArray(obj)) {
      nextTok = obj.length;
    }
    obj[nextTok] = value;
    return this;
};	
