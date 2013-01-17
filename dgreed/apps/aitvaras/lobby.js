var http = require('http');
var fs = require('fs');
var crypto = require('crypto');
var path = require('path');
var url = require('url');

// reading lobby port number from config file
var lobby_port = getLobbyPort();

function getLobbyPort(){
	var fs = require('fs');

	var temp = 80;
	var data = fs.readFileSync('./aitvaras_scripts/config.lua', 'utf8');
	var lines = data.split("\n");
	for (var i in lines) {
		var regex = /^(?!--)config.lobby_port/;
		var result = lines[i].match(regex);
		if(result !== null){
			temp = parseInt(lines[i].replace(/.*?=/,''));
		}
	}
	if(temp == 80) console.log('Lobby port is undefined in the config file, using default: 80');
	return temp;
}

function startsWith(str, prefix) {
	return str.indexOf(prefix) === 0;
}

function genId(addr) {
	var hash = crypto.createHash('sha1');
	hash.update(addr);
	var n = parseInt(hash.digest('hex'), 16);
	return n % 10000;
}

function getIp(req) {
	return req.headers['x-forwarded-for'] || req.connection.remoteAddress;
}

// Our "database" for id = addr pairs
var servers = {}

http.createServer(function (req, res) {

	if(req.url === '/')
		req.url = '/index.html'

	var rurl = url.parse(req.url);
	req.url = rurl.pathname;

	console.log(req.url);

	if(req.method === 'POST' && req.url === '/enlist') {
		// Enlist a new game server

		var addr = '';
		req.on('data', function(chunk) {
			addr += chunk.toString();
		});

		req.on('end', function() {
			// Use ip if there was no address
			if(addr === '')
				addr = getIp(req);

			var id = genId(addr);
			console.log('Enlisting server ' + id + ' = ' + addr);
			
			if(typeof servers[id] === 'string') {
				if(servers[id] !== addr) {
					console.log('Hash collision?');
					res.writeHead(500);
					res.end();
					return;
				}
			}

			servers[id] = addr;

			res.writeHead(200, {'Content-Type': 'text/plain'});
			res.end(id.toString());
		});
	}
	else if(req.method === 'POST' && req.url === '/remove') {
		// Remove game server
		
		var addr = '';
		req.on('data', function(chunk) {
			addr += chunk.toString();
		});

		req.on('end', function() {
			// Use ip if there was no address
			if(addr === '')
				addr = getIp(req);

			var id = genId(addr);
			console.log('Remove server ' + id + ' = ' + addr);
			
			if(servers[id] !== addr) {
				console.log('No such server to remove');
				res.writeHead(500);
				res.end();
				return;
			}

			servers[id] = undefined;

			res.writeHead(200);
			res.end();
		});
	}
	else if(req.method === 'GET') {
		var filePath = './aitvaras_html' + req.url;
		path.exists(filePath, function(exists) {
			if(exists) {
				var extname = path.extname(filePath);
				var contentType = 'text/html';
				switch(extname) {
					case '.json':
						contentType = 'application/json';
						break;
					case '.png':
						contentType = 'image/png';
						break;
					case '.css':
						contentType = 'text/css';
						break;
					case '.js':
						contentType = 'text/javascript';
						break;
				}

				fs.readFile('./aitvaras_html' + req.url, function(error, content) {
					if(error) {
						res.writeHead(500);
						res.end();
					}
					else {
						res.writeHead(200, {'Content-Type': contentType});
						res.end(content, 'utf-8');
					}
				});
			}
			else {
				try {
					var id = parseInt(req.url.substring(1));
					if(typeof id !== 'number' || typeof servers[id] !== 'string')
						throw 'bad addr or no such server';

					console.log('Server for id ' + id + ' = ' + servers[id]);

					res.writeHead(200, {
						'Content-Type': 'text/plain',
						'Access-Control-Allow-Origin' : '*' 
					});
					res.end(servers[id]);
				}
				catch(err) {
					console.log('Error in GET request');
					res.writeHead(404);
					res.end();
				};
			}
		});
	}
}).listen(lobby_port);

