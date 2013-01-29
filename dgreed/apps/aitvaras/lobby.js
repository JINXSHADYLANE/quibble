var http = require('http');
var fs = require('fs');
var crypto = require('crypto');
var path = require('path');
var url = require('url');

var router = require('router');
var route = router();

// reading lobby port number from config file
var lobby_port = getLobbyPort();
var lobby_addr = getLobbyAddr();

function getLobbyPort(){
	var fs = require('fs');
	var temp = -1;
	var data = fs.readFileSync('./aitvaras_scripts/config.lua', 'utf8');
	var lines = data.split("\n");
	for (var i in lines) {
		var regex = /^(?!--)config.lobby_port/;
		var result = lines[i].match(regex);
		if(result !== null){
			temp = parseInt(lines[i].replace(/.*?=/,''));
		}
	}
	if(temp == -1){
		console.log('Lobby port is undefined in the config file, using default: 80');
		temp = 80;
	}
	return temp;
}

function getLobbyAddr(){
	var fs = require('fs');
	var temp = '';
	var data = fs.readFileSync('./aitvaras_scripts/config.lua', 'utf8');
	var lines = data.split("\n");
	for (var i in lines) {
		var regex = /^(?!--)config.lobby_addr/;
		var result = lines[i].match(regex);
		if(result !== null){
			temp = lines[i].replace(/.*?'/,'');
			temp = temp.replace(/'/,'');
		}
	}
	if(temp == '') console.log('Lobby adress is undefined in the config file');
	return temp;
}

// replacing lobby address
fs.readFile('./aitvaras_html/index.html', 'utf8', function (err,data) {
	if (err) {
		return console.log(err);
	}
	var result = data.toString();
	var lobbyString = 'var lobby_addr = "' + lobby_addr + '"';
	result = result.replace(/var lobby_addr = ".*"/, lobbyString);
	
	fs.writeFile('./aitvaras_html/index.html', result, 'utf8', function (err) {
		if (err) return console.log(err);
	});
	console.log('ready');
});

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

function getContent(req, res, contentType){
	console.log(req.url);
	var filePath = './aitvaras_html' + req.url;
	path.exists(filePath, function(exists) {
		if(exists) {
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
		} else {
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

route.get(/\.css(?!\.)/i, function(req, res) {
	getContent(req,res,'text/css');
});
route.get(/\.png(?!\.)/i, function(req, res) {
	getContent(req,res,'image/png');
});	
route.get(/\.json(?!\.)/i, function(req, res) {
	getContent(req,res,'application/json');
});	
route.get(/\.js(?!\.)/i, function(req, res) {
	getContent(req,res,'text/javascript');
});

route.get(function(req, res) {
	if(req.url === '/')
		req.url = '/index.html'
	var rurl = url.parse(req.url);
	req.url = rurl.pathname;
	
	getContent(req,res,'text/html');
});

route.post('/enlist', function(req, res) {
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
});

route.post('/remove', function(req, res) {
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
});		

http.createServer(route).listen(lobby_port);
