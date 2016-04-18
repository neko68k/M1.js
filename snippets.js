Module['preRun'] = function() {
	FS.createFolder('/', 'roms', true, true)
	FS.createPreloadedFile('/', 'm1.xml', 'http://localhost/m1.xml', true, false);
	FS.createPreloadedFile('/roms', '1941.zip', 'http://localhost/roms/1941.zip', true, false);
	Module['arguements']=['1941'];
};

var gamejmp = Module.cwrap('gamejmp', null, ['number']);
var findgame = Module.cwrap('findgame', 'string', ['number']);
var gamenum = findgame('mslug');
gamejmp(gamenum);

function getParameterByName(name, url) {
	if (!url) url = window.location.href;
	name = name.replace(/[\[\]]/g, "\\$&");
	var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)", "i"),
	results = regex.exec(url);
	if (!results) return null;
	if (!results[2]) return '';
	return decodeURIComponent(results[2].replace(/\+/g, " "));
}

var romname = getParameterByName('rom')









