
JSON-bag

An exercise of Web tools for supporting the embedding of Binary data in JSON responses with the following objectives:

- avoid base64 bandwidth and decoding overhead
- avoid custom encodings such MessagePack that are slow in browser decoding
- reduce number of requests or additional production logic (if not using HTTP 2.0 Server Push)
- can be used also for offline storage

Usage scenario: rich content produced on the fly from an embedded web server.

# Status

## Not Done

- URL split mode in server (client is automatic)
- emit chunked HTTP (JSON and then blobs)
- headers for blobs
- progressive
- deserialization in C++

# Approach

Create an JSON-bag response the acts much like a multipart MIME with binary data. We would like to have a solution that is human-readable (for the JSON part) and then directly addressable in the browser.

Having found that [https://github.com/KhronosGroup/glTF/tree/master/extensions/Khronos/KHR_binary_glTF](Binary glTF (KHR_binary_glTF)) addresses a similar problem JSON-bag provides a general solution.

Apart the Binary glTF specific domain the differences are:
- patching of the JSON for supporting resource access
- readable JSON heading

## HTTP Transfer

Logical structure:

- header
- JSON
- crlf null
- blob1: header + data
- blob2: header + data
- ...

The first header contains only the size in bytes of the JSON part.

The headers part in the above structure could be stored in the HTTP headers so that the initial part of the response looks like as JSON. Alternative we can put the size in non-binary form as first line (e.g. textual magic + size in hexadecimal followed by crlf).

The blob header contains:
- size
- content-type
- JSON path for patching the JSON

As done in Binary glTF we try to enforce alignment of blob content

## File Serialization

The JSON-bag could be stored as a JSON file plus one or more binary files (as glTF 2.0 does) or a single file. In this case the first header has to be stored in the file.

# Browser Side

The requests pulls AJAX data with special Accept header. Then receives the response and decodes first the JSON then uses the recently added Blob objects for creating views of data over the ArrayBuffer

	var blob = new Blob( [ arrayBufferView ], { type: "image/jpeg" } );
	var urlCreator = window.URL || window.webkitURL;
	image.src = urlCreator.createObjectURL( blob );

The browser does the following:

- read the JSON as part of the array buffer received using the size
- read the declarations of binary parts, creates Blob object, obtain the URL and then patches the JSON in the correct path

# Server Side

Example in C++ using Mongoose. The C++ API is independent of the JSON-bag and can produce the single JSON-bag or a JSON with multiple URLs.  If the Accept header is present it serves the JSON-bag otherwise the server can decide between split URL version or base64 embedding. In anycase the URL in the JSON content is patched.

In the split version it patches the JSON with the effective URL, while in the single version it puts a placeholder that contains offset and size to the binary output.

Note the server could take the serialized version of JSON-bag and serve it.

## URL in JSON

In JSON-bag different URL types are used in the JSON depends the transfer:

- data: the classic base64 embedding, the simplest scenario
- http: storing the contents in separate URL
- blob: the new URL type supported by the Blob class and created as a view over the ArrayBuffer
- binary: a custom URL that is used to mark the offset, size and mime of an entity wrt the binary part

# HTTP 2.0

In HTTP 2.0 there is the possibility of Server Push that is sending additional content together with a response. In this scenario JSON-bag is straightforward: the server produces the first answer as JSON containing the references to the (virtual) URL pushed together the JSON.

# References

## Normative References
- JSON Path RFC
- JSON RFC 
- HTTP 2.0
- KHR_binary_glTF

And notes: https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/Sending_and_Receiving_Binary_Data

## Binary Manipulation in JS

- https://github.com/jDataView/jDataView
- https://github.com/jDataView/jBinary