var sqlite3 = require("sqlite3");
var db = new sqlite3.Database('/opt/gateway/temp.db');
var apn = require("apn");
var tokens = ["applepushotificationstoken"];
var serialport = require("serialport");
var emitcheck = 0;
var serial = new serialport.SerialPort("/dev/ttyUSB-A601SRAD", { baudrate : 115200, parser: serialport.parsers.readline("\n") });
serial.on("data", function (data) {
	//console.log(data);
	var status = '';
	if (data.indexOf('{') == 0) {
		var r = JSON.parse(data);
		if (r['nodeId'] == 7) {
			io.sockets.emit('Laundry', r);
			if (r['dryerAlert'] == 1) {
			    //pushNotificationToMany("Dryer Done.");
			} 
			if (r['washerAlert'] == 1) {
			    //pushNotificationToMany("Washer Done.");
			} 

		}
		// id of weather node
		if (r['nodeId'] == 9) {
			if (emitcheck != r['uptime']) {
				io.sockets.emit('Weather', r);
				var dt = new Date();
				var now = dt.getFullYear() + "/" + (dt.getMonth()+1) + "/" + dt.getDate();
				var stmt = db.prepare("INSERT INTO weather (temp, humidity, pressure, light, date) VALUES (?, ?, ?, ?, ?)");
        			stmt.run(r['tempreture'], r['humidity'], r['pressure'], r['light'], now);
        			stmt.finalize();
				emitcheck = r['uptime'];
			}		
		}
	}
  	if (data.indexOf(' OPEN ') != -1)  status = 'Open';
  	if (data.indexOf(' CLOSED ') != -1) status = 'Closed';
  	if (data.indexOf(' OPENING ') != -1) status = 'Opening';
  	if (data.indexOf(' CLOSING ') != -1) status = 'Closing';
  	if (data.indexOf(' UNKNOWN ') != -1) status = 'Unknown';
  	if (status.length > 0) {
		console.log(status);
		io.sockets.emit('GarageDoor', status);
  	}
	//console.log(data);
});

var express = require('express');
var fs = require('fs');
var https = require('https');
var http = require('http');
// generate you own certs 
var opts = { key: fs.readFileSync('/opt/gateway/certs/server.pem'), cert: fs.readFileSync('/opt/gateway/certs/server.crt') };

var app = express(opts);
app.set('trust proxy', true);
//var app = express();
app.set('views', __dirname + '/views');
app.use(express.static(__dirname + '/public'));
app.engine('html', require('ejs').renderFile);

app.use(express.json());
app.use(express.urlencoded());

var server = https.createServer(opts, app).listen(8443);
//var server = http.createServer(app).listen(8443);
var io = require('socket.io').listen(server);

//start listening for connections socket.io
io.sockets.on('connection', function(socket) {

});

/*
var service = new apn.connection({ cert:'/opt/gateway/certs/apn-cert.pem', key:'/opt/gateway/certs/apn-key.pem', gateway:'gateway.sandbox.push.apple.com' });
service.on('connected', function() {
    console.log("Connected");
});

service.on('transmitted', function(notification, device) {
    console.log("Notification transmitted to:" + device.token.toString('hex'));
});

service.on('transmissionError', function(errCode, notification, device) {
    console.error("Notification caused error: " + errCode + " for device ", device, notification);
});

service.on('timeout', function () {
    console.log("Connection Timeout");
});

service.on('disconnected', function() {
    console.log("Disconnected from APNS");
});

service.on('socketError', console.error);

function pushNotificationToMany(msg) {
    service = new apn.connection({ cert:'/opt/gateway/certs/apn-cert.pem', key:'/opt/gateway/certs/apn-key.pem', gateway:'gateway.sandbox.push.apple.com' });
    var note = new apn.notification();
    note.setAlertText(msg);
    note.badge = 1;
    note.sound = "LaundryAlert.aiff";
    service.pushNotification(note, tokens);
    service.shutdown();
}*/
app.get('/api/:nodeid/status', function(req, res) {

});

app.get('/api/:id', function(req, res) {
	serial.write('GRGSTS');	
	res.setHeader('content-type', 'application/json');
	res.send('{"status" : "success"}');
});

app.post('/api/:action', function(req, res) {
	if (req.params.action == "open") {
		serial.write('GRGOPN');	
	} else  {
		serial.write('GRGCLS');		
	}
	res.setHeader('content-type', 'application/json');
	res.send('{"status" : "success", "action" : "close"}');
});


app.get('/', function(req, res) {
	res.render('index.html');
});
