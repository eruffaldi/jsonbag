//npm install express --save
const express = require('express')  
const jsonbag = require('jsonbag_serverside')
const app = express()  
const port = 8000



function makejsonbag(request,response,usebin,tofile)
{
  var jb = new jsonbag.JSONBagBuilder()
  jb.setInlineMode(!usebin)
  {
    var r = [];
    r.push({name: "image1", url: ""})
    jb.assignFile(r[0],"url","/0/url","image/png","logo.png",false)
    r.push({name: "image2", url: ""})
    jb.assignFile(r[1],"url","/1/url","image/png","logo.png",false)
  }
  if(!tofile)
  {
    jb.serialize_express(response)
  }
  else
  {
    jb.serialize_file(tofile)    
  }
}

app.get('/json', (request, response) => {  
  response.send('Hello from Express!')
  makejsonbag(request,response,false,"")
})

app.get('/jsonbin', (request, response) => {  
  response.send('Hello from Express!')
  makejsonbag(request,response,true,"")
})

app.get('/jsonlocal', (request, response) => {  
  response.send('Hello from Express!')
  makejsonbag(request,response,false,"output.json")
})

app.get('/jsonbinlocal', (request, response) => {  
  response.send('Hello from Express!')
  makejsonbag(request,response,true,"output.jsonbag")
})

app.use(express.static('.'))

app.listen(port, (err) => {  
  if (err) {
    return console.log('something bad happened', err)
  }

  console.log(`server is listening on ${port}`)
})