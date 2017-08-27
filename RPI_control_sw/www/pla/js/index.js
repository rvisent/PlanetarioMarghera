/******************************************

index.js

@author: Marco Visentin

******************************************/

var strdata_old;
var oldMode = -1;

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
		switch (data.PlaMode) {
			case MODE.MODE_IDLE:
				if (oldMode != data.PlaMode) {
					// nascondi bottone abort
					$("#abort").hide();
					oldMode = data.PlaMode;
				}
				break;
			case MODE.MODE_ZERO:
			case MODE.MODE_SAFEPOS:
				if (oldMode != data.PlaMode) {
					// mostra bottone abort
					$("#abort").show();
					oldMode = data.PlaMode;
				}
				break;
			case MODE.MODE_RUN:
				// torna alla pagina run!
				window.location.replace("simulation.html");
				return;
			case MODE.MODE_TEST:
				// apri la pagina test
				window.location.replace("test.html");
				return;
			case MODE.MODE_SETPLANETS:
				// apri la pagina impostazione pianeti
				window.location.replace("planets.html");
				return;
			// in tutti gli altri casi non serve fare nulla
		}

		var safe = (data.latitude_now >= 68 && data.latitude_now <= 72 && data.bZeroDone == 1) ? "Sì".toUpperCase():"NO";
		var zero = (data.bZeroDone == 1) ? "Sì".toUpperCase():"NO";
		document.getElementById("data").innerHTML = '<li><p>Posizione sicura</p><pre>' + safe + '</pre></li>' +
													'<li><p>Azzerato</p><pre>' + zero + '</pre></li>' +
													'<li><p>Modo: </p><pre>' + getModeString(data.PlaMode) + '</pre></li>';
		
		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});
}

//Safepos
function safepos() {
	$.post(
		"/planetario.fcgi",
		{
			"NewMode": MODE.MODE_SAFEPOS
		}
	);
}

//Force zero
function zeropos() {
	$.post(
		"/planetario.fcgi",
		{
			"NewMode": MODE.MODE_ZERO
		}
	);
}

//Shutdown
function shutdown() {
	$.post(
		"/planetario.fcgi",
		{
			"NewMode": MODE.MODE_SHUTDOWN
		}
	);
}
