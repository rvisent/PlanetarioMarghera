/******************************************

test.js

@author: Marco Visentin

******************************************/
var strdata_old;
var testdata_old;
var first_time = 1;
var oldMode = -1;
var motorSpeedElement, motorTargetElement;

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
			case MODE.MODE_RUN:
				// torna alla pagina run!
				window.location.replace("simulation.html");
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
		
		// al primo evento timer entra in modo test e attiva TestTimer
		if (first_time) {
			first_time = 0;
			$.post("/planetario.fcgi", {"NewMode": MODE.MODE_TEST});

			// inizializza elementi per accesso in test()
			motorSpeedElement = document.getElementById("motorSpeed");
			motorTargetElement = document.getElementById("motorTarget");

			// attiva il primo timeout di TestTimer
			setTimeout(TestTimer, 100);
		}

		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});
}


function TestTimer() {
	$.getJSON("/planetario.fcgi?test", function (data, status) {
		strdata = JSON.stringify(data);
		if (strdata == testdata_old) {
			// se non ci sono cambiamenti, usa timeout più lungo (diverso da AjaxTimer,
			//	 così tendono a non sovrapporsi)
			setTimeout(TestTimer, 235);
			return;
		}
		testdata_old = strdata;

		// aggiorna le letture
		$("#motorSpeed").attr("placeholder", data.motorSpeed);
		$("#decl").html(data.declinazione);
		$("#ascRetta").html(data.ascRetta);
		$("#ascRettaOffset").html(data.ascRettaOffset);
		$("#precessione").html(data.precessione);
		$("#motorTarget").attr("placeholder", data.motorTarget);

		// dati cambiati, accelera il campionamento
		setTimeout(TestTimer, 100);
	});
}

// Motors test
function test(par) {
	var motorId = $('input[name=motor]:checked').val();
	if (motorId == null || motorId < 0 || motorId > 3) {
		alert("Seleziona un'opzione!");
	}
	else
	{
		var motorSpeed = motorSpeedElement.value;
		if (motorSpeed == null || motorSpeed == undefined)
			motorSpeed = motorSpeedElement.placeholder;
		var motorTarget = motorTargetElement.value;
		if (motorTarget == null || motorTarget == undefined)
			motorTarget = motorTargetElement.placeholder;
		
		var json_struct = {"motorId": motorId, "motorSpeed": motorSpeed, "motorTarget": motorTarget};
		// add command (key on a var, not a literal)
		json_struct["bTest" + par] = 1;

		$.post("/planetario.fcgi", json_struct);
	}
}
