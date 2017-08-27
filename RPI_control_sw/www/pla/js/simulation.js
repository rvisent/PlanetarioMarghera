/******************************************

simulation.js

@author: Marco Visentin

******************************************/
var strdata_old;
var oldMode = -1;
var beepDone = false;

function AjaxTimer() {
	$.getJSON("/planetario.fcgi?mainset", function (data, status) {
		strdata = JSON.stringify(data);
		if (strdata == strdata_old) {
			// se non ci sono cambiamenti, usa timeout più lungo
			setTimeout(AjaxTimer, 250);
			return;
		}
		strdata_old = strdata;

		// visualizza/resetta errori
		checkError(data.idError, "");

		// cambia pagina se errata, oppure resetta il modo
		// inoltre aggiorna il bottone play/pausa/continua
		switch (data.PlaMode) {
			case MODE.MODE_IDLE:
				// predisponi bottone RUN e nascondi ABORT. Evita refresh continui della proprietà
				if (oldMode != data.PlaMode) {
					$("#play-button").html('<button id="play" onclick="run()"><i class="fa fa-play fa-lg"></i></button>');
					// nascondi bottone abort
					$("#abort").hide();
					oldMode = data.PlaMode;
				}
				break;
			case MODE.MODE_ZERO:
				// predisponi bottoni PAUSA e ABORT. Evita refresh continui della proprietà
				if (oldMode != data.PlaMode) {
					$("#play-button").html('<button id="play" onclick="pause()"><i class="fa fa-pause fa-lg"></i></button>');
					// visualizza bottone abort
					$("#abort").show();
					oldMode = data.PlaMode;
				}
				break;
			case MODE.MODE_RUN:
				// predisponi bottoni pausa/continua e ABORT, usa un valore speciale per gestire
				//   le due condizioni pausa/continua
				if (data.bPaused && oldMode != MODE.MODE_RUN+100) {
					$("#play-button").html('<button id="play" onclick="play()"><i class="fa fa-rotate-right fa-lg"></i></button>');
					// visualizza bottone abort
					$("#abort").show();
					oldMode = MODE.MODE_RUN+100;
				}
				else if (!data.bPaused && oldMode != MODE.MODE_RUN) {
					$("#play-button").html('<button id="play" onclick="pause()"><i class="fa fa-pause fa-lg"></i></button>');
					// visualizza bottone abort
					$("#abort").show();
					oldMode = MODE.MODE_RUN;
				}
			    // fai beep entrando nei modi "pronto"
				if (data.PlaSubMode == SUBMODE.PRONTO_POMERIGGIO || data.PlaSubMode == SUBMODE.PRONTO_NOTTE || data.PlaSubMode == SUBMODE.PRONTO_ALBA) {
				    if (!beepDone) {
				        // fai solo una volta
				        PlaySound("beep");
				        beepDone = true;
				    }
				}
				else {
				    beepDone = false;
				}
                break;
			case MODE.MODE_TEST:
				// apri la pagina test
				window.location.replace("test.html");
				return;
			case MODE.MODE_SETPLANETS:
				// apri la pagina impostazione pianeti
				window.location.replace("planets.html");
				return;
			case MODE.MODE_OFFLINE:
			case MODE.MODE_SHUTDOWN:
				// torna alla pagina principale
				window.location.replace("index.html");
				return;
			// in tutti gli altri casi non serve fare nulla
		}

		var state = getModeString(data.PlaMode);
		var run_status = getSubmodeString(data.PlaSubMode, data.perc_action);

		document.getElementById("infos").innerHTML = '<li><p>Ascensione retta: </p><pre>' + data.ar_nowpr.toFixed(2) + '</pre></li>' +
													 '<li><p>Latitudine: </p><pre>' + data.latitude_now + '</pre></li>' +
													 '<li><p>Ora: </p><pre>' + format(data.time_hour, data.time_min) + '</pre></li>' +
													 '<li><p>Precessione anno: </p><pre>' + data.prec_now_year.toFixed(0) + '</pre></li>' +
													 '<li><p>Modo: </p><pre>' + state + '</pre></li>' +
													 '<li><p>Stato: </p><pre>' + run_status + '</pre></li>';

		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});
}

// Start simulation
function run() {
	$.post("/planetario.fcgi", {"NewMode": MODE.MODE_RUN});
}

// Pause
function pause() {
	$.post("/planetario.fcgi", {"bSuspend": 1});
}

// Play
function play() {
	$.post("/planetario.fcgi", {"bContinue": 1});
}

function format(h, min) {
	if (min < 10) min = "0" + min;
	
	return n = h + ":" + min;
}
