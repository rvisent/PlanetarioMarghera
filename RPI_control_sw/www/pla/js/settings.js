/******************************************

settings.js

@author: Marco Visentin

******************************************/
var strdata_old;
var bRefresh=1;

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
		
		// aggiorna i placeholder solo all'inizio, così non vengono
		//   riscritti durante l'editing
		if (bRefresh) {
/*			$("#annoPrecessione").attr("placeholder", data.annoPrecessione);
			$.getJSON("/planetario.fcgi?auxiliary", function (auxdata, status) {
				$("#latitude").attr("placeholder", auxdata.latitude);
				$("#horlight").attr("placeholder", auxdata.horlight);
				$("#maxstars").attr("placeholder", auxdata.maxstars);
				$("#speed").attr("placeholder", auxdata.speed);
				$("#sun_ar").attr("placeholder", auxdata.sun_ar);
				$("#moon_ar").attr("placeholder", auxdata.moon_ar);
				$("#mercury_ar").attr("placeholder", auxdata.mercury_ar);
				$("#venus_ar").attr("placeholder", auxdata.venus_ar);
				$("#mars_ar").attr("placeholder", auxdata.mars_ar);
				$("#jupiter_ar").attr("placeholder", auxdata.jupiter_ar);
				$("#saturn_ar").attr("placeholder", auxdata.saturn_ar);
				bRefresh=0;
			});
*/
			$("#annoPrecessione").val(data.annoPrecessione);
			$.getJSON("/planetario.fcgi?auxiliary", function (auxdata, status) {
				$("#latitude").val(auxdata.latitude);
				$("#horlight").val(auxdata.horlight);
				$("#maxstars").val(auxdata.maxstars);
				$("#speed").val(auxdata.speed);
				$("#sun_ar").val(auxdata.sun_ar);
				$("#moon_ar").val(auxdata.moon_ar);
				$("#mercury_ar").val(auxdata.mercury_ar);
				$("#venus_ar").val(auxdata.venus_ar);
				$("#mars_ar").val(auxdata.mars_ar);
				$("#jupiter_ar").val(auxdata.jupiter_ar);
				$("#saturn_ar").val(auxdata.saturn_ar);
				bRefresh=0;
			});
		}

		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});

}
