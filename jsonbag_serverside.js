var fs = require('fs');

// JS data64
// JS byte buffer
/*
 Buffer.from(array)
 .toString("base64")
Buffer.from(other[, byteOffset [, length]])
Buffer.alloc(n)
Buffer.concat

https://nodejs.org/docs/latest/api/fs.html#fs_class_fs_readstream
https://nodejs.org/docs/latest/api/fs.html
https://nodejs.org/docs/latest/api/buffer.html#buffer_new_buffer_array
 */
function JSONBagBlock(s,o,p)
{
  this.size = s
  this.offset = o
  this.prefix = p
  this.path = ""
  this.mime = ""
  this.memory = ""
  this.filename = ""
}

JSONBagBlock.prototype.buildPrefix = function()
{
	var s = JSON.stringify({ path : this.path,size:this.size,mime:this.mime});
	b.prefix =s;
	var hs = s.length+2

	var buf = Buffer.allocUnsafe(2)
	buf.writeUInt16BE(s.length, 0);
    this.prefix = buf.toString() + s
}

JSONBagBlock.prototype.buildUrl = function()
{
	return "linkdata:" + this.mime + ";offset=" + this.offset + ";size=" + this.size
}

function JSONBagBuilder()
{
  this.inline = false
  this.blocks = []
  this.offset = 0
}

JSONBagBuilder.prototype.setInlineMode = function (inline)
{
  this.inline = inline
}

JSONBagBuilder.prototype.assignData = function (obj,item,path,mime,data)
{

	if(this.inline)
	{
		var b =  new Buffer(data)
		obj[item] = "data:" + mime + ";base64," + b.toString("base64")
	}
	else
	{
		var b = new JSONBagBlock()
		b.size = data.length
		b.offset = this.offset
		b.path = path
		b.data = data
		b.buildPrefix()
		this.offset += b.size + 2 + b.prefix.length
		this.blocks.push(b)
		obj[item] = b.buildUrl()
	}
}

// TODO automatic mime type
JSONBagBuilder.prototype.assignFile = function (obj,item,path,mime,filename,deferred)
{
	if(deferred)
	{
		if(this.inline)
		{
			obj[item] = "data:" + mime + ";base64," + fs.readFileSync(filename,{encoding:"base64"});
		}
		else
		{
			var b = new JSONBagBlock(fs.statSync(filename).size,this.offset,path)
			b.filename = filename
			b.buildPrefix();
			e = b.buildUrl();
			currentoff += b.effectiveSize();
			this.offset += b.size + 2 + b.prefix.length
			this.blocks.push(b)
			obj[item] = b.buildUrl()
		}			
	}
	else 
	{
		if(this.inline)
		{
			obj[item] = "data:" + mime + ";base64," + fs.readFileSync(filename,{encoding:"base64"});
		}
		else
		{
			var b = new JSONBagBlock(fs.statSync(filename).size,this.offset,path)
			b.memory = fs.readFileSync(filename,{encoding:"base64"})
			this.offset += b.memory + size + 2 + b.prefix.length
			this.blocks.push(b)
			obj[item] = b.buildUrl()
		}
	}
}

JSONBagBuilder.prototype.payloadLength = function()
{
	return this.offset
}


// req.accepts("...")
/*
https://expressjs.com/en/4x/api.html#req.xhr
res.sendStatus(statusCode)
res.set(field [, value])

res.end([data] [, encoding])
res.json([body])
res.send([body])
*/
JSONBagBuilder.prototype.serialize_express = function (response)
{
	if(this.inline)
	{
		var s = JSON.stringify(this.root)
		response.set("Content-Length",s.length)
		response.set("Content-Type","application/json")
		response.write(s,"binary")
	}
	else
	{
		var s = JSON.stringify(this.root)
		var h = s.length.toString(16)
		var sep = "\r\n\x00"
		var firstlinesize = (5+2+h.length)
		var totalsize = sep.length+s.length+ firstlinesize+ this.payloadLength()
		response.set("Content-Type","application/jsonbag")
		response.set("Content-Length",totalsize)
		response.set("SONBag-Range: " + firstlinesize + " " + s.size())
		response.write("JBAG00" + h + "\r\n","binary")
		response.write(s,"binary")
		response.write(sep,"binary")


		// TODO: make this totally asynchronous as a chain of following operations
		//
		// emit header
		// emit file
		// pad file
		// emit header
		// ....
		//
		// var stream = fs.createReadStream(path, options)
		// stream.on('end', function onend () {    self.emit('end')  })
		//  copy b.filename cut/pad to b.size
		// See https://github.com/pillarjs/send/blob/master/index.js
    	var buf = new Buffer(16384)
		for(var i = 0; i < this.blocks.length; i++)
		{
			var b = this.blocks[i]
			response.write(b.prefix)
	        if(b.filename)
	        {
	        	var fd = fs.openSync(b.filename,"r")
	        	var n = b.size
	        	while(n > 0)
	        	{
	        		var done = fs.readSync(fd,buf,0,n < buf.length ? n : buf.length)
	        		if(done == 0)
	        			break;
	        		// write full or part
	        		if(done < buf.length)
		        		response.write(new Buffer(buf,0,done))
		        	else
		        		response.write(buf)
	        	}

	        	// pad if file changed size
	        	if(n > 0)
	        	{
	        		// fill up to buffer
	        		buf.fill(0,0,Math.min(n,buf.length),"binary")
		        	while(n > 0)
		        	{
		        		if(n >= buf.length)
		        		{
		        			response.write(buf)
		        			n -= buf.length
		        		}
		        		else
		        		{
		        			response.write(new Buffer(buf,0,n))
		        			break
		        		}
		        	}
		        }
	        }
	        else
	        {
				// should we enforce b.size?
	        	response.write(b.memory,"binary")
	        }
		}
	}
}

JSONBagBuilder.prototype.serialize_file = function (file)
{
	var wrapfile = { write: function() { return file.write.apply(file,arguments)}, set: function (n,v) {} }
	this.serialize_express(wrapfile)
}

exports.JSONBagBuilder = JSONBagBuilder