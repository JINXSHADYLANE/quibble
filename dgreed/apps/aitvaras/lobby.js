var http = require('http');
var fs = require('fs');
var crypto = require('crypto')

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
	else if(req.method === 'GET' && startsWith(req.url, '/')) {
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
	else {
		console.log('Unrecognized request ' + req.url);
		res.writeHead(404);
		res.end();
	}
}).listen(80);

