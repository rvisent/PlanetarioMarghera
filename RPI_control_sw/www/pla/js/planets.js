/******************************************

planets.js

@author: Marco Visentin

******************************************/
var strdata_old;
var PlaMode;
var oldMode = -1;
var first_time = 1;

function AjaxTimer() {
	$.getJSON("/planetario.fcgi?mainset", function (data, status) {
		strdata = JSON.stringify(data);
		if (strdata == strdata_old) {
			// se non ci sono cambiamenti, usa timeout più lungo
			setTimeout(AjaxTimer, 250);
			return;
		}
		strdata_old = strdata;

		PlaMode = data.PlaMode;

		// visualizza/resetta errori
		checkError(data.idError, "");

		// cambia pagina se errata, oppure inizializza il modo
		switch (data.PlaMode) {
			case MODE.MODE_IDLE:
				if (oldMode != data.PlaMode) {
					// nascondi bottone abort
					$("#abort").hide();
					oldMode = data.PlaMode;
				}
				break;
			case MODE.MODE_ZERO:
			case MODE.MODE_SETPLANETS:
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
			case MODE.MODE_OFFLINE:
			case MODE.MODE_SHUTDOWN:
				// torna alla pagina principale
				window.location.replace("index.html");
				return;
			// in tutti gli altri casi non serve fare nulla
		}
		
		if (first_time) {
			first_time = 0;
			// inizializza le AR dei pianeti
			$.getJSON("/planetario.fcgi?auxiliary", function (auxdata, status) {
				$("label[for='SET_SOLE']").append(formatAR(auxdata.sun_ar));
				$("label[for='SET_LUNA']").append(formatAR(auxdata.moon_ar));
				$("label[for='SET_MERCURIO']").append(formatAR(auxdata.mercury_ar));
				$("label[for='SET_VENERE']").append(formatAR(auxdata.venus_ar));
				$("label[for='SET_MARTE']").append(formatAR(auxdata.mars_ar));
				$("label[for='SET_GIOVE']").append(formatAR(auxdata.jupiter_ar));
				$("label[for='SET_SATURNO']").append(formatAR(auxdata.saturn_ar));
				bRefresh=0;
			});
		}

		var state = getModeString(data.PlaMode);
		document.getElementById("set-data").innerHTML = '<li><p>Ascensione retta: </p><pre>' + data.ar_nowpr.toFixed(2) + '</pre></li>' +
														'<li><p>Precessione: </p><pre>' + data.prec_now_year.toFixed(2) + '</pre></li>' +
														'<li><p>Modo: </p><pre>' + state + '</pre></li>';

		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});
}

// Mode -> SETTINGS + AR settings
function set() {
	var selected = $('input[name=planet]:checked').val();
	if (selected == null || selected<0 || selected > SETMODE.SET_SATURNO) {
		alert("Seleziona un'opzione!");
	}
	else
	{
		if (PlaMode != MODE.MODE_SETPLANETS)
			$.post("/planetario.fcgi", {"NewMode": MODE.MODE_SETPLANETS, "NewSettingMode": selected});
		else
			$.post("/planetario.fcgi", {"NewSettingMode": selected});
	}
}

function formatAR(ar) {
	// | 0 does truncation (forces conversion to int32)
	hrs = ar | 0;
	min = ((ar-hrs)*100.+0.005) | 0;
	sec = (((ar-hrs)*100.-min)*100.+0.5) | 0;
	return ' (AR= '+hrs+'h '+min+"' "+sec+'")';
}
