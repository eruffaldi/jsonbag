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
function JSONBagBlock()
{
  this.size = 0
  this.offset = 0
  this.prefix = ""
  this.path = ""
  this.mime = ""
  this.managed = ""
  this.filename = ""
}

//JSONBagBlock.prototype. = function()

function JSONBagBuilder()
{
  this.inline = false
  this.blocks = []
}

JSONBagBuilder.prototype.setInlineMode = function (inline)
{
  this.inline = inline
}

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
			var b = new JSONBagBlock()


			this.blocks.push(b)
			obj[item] = "linkdata:" + mime + ";offset=" + b.offset + ";size=" + b.size			
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
			var b = new JSONBagBlock()

			this.blocks.push(b)
			obj[item] = "linkdata:" + mime + ";offset=" + b.offset + ";size=" + b.size
		}
	}
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
	for(var i = 0; i < this.blocks.length; i++)
	{
		
	}
}

JSONBagBuilder.prototype.serialize_file = function (file)
{
	for(var i = 0; i < this.blocks.length; i++)
	{

	}
}

exports.JSONBagBuilder = JSONBagBuilder