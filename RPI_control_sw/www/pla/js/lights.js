/******************************************

lights.js

@author: Marco Visentin

******************************************/
var strdata_old;
var lightdata_old;
var light_code = 0;
var oldMode = -1;
var sliding = 0;
var PlaMode;
var light_low, light_med, light_high;
var beepDone = false;


// usa DOMready callback per installare slider callback e attivare LightTimer
$(function () {
    $("#slider").slider();
    $("#slider").on("slidestop", function (event, ui) {
        sliding = 0;
        $.post("/planetario.fcgi", { "lightLevel": ui.value, "lightToBeChanged": lightcode });
        // forza un timeout rapido per vedere subito l'aggiornamento
        setTimeout(LightTimer, 50);
    });
    $("#slider").on("slidestart", function (event, ui) {
        sliding = 1;
    });
    $("#selector").change(function () {
        sliding = 0;
        var value;
        switch (this.selectedIndex) {
            case 0: value = 0; break;
            case 1: value = light_low; break;
            case 2: value = light_med; break;
            case 3: value = light_high; break;
        }
        $.post("/planetario.fcgi", { "lightLevel": value, "lightToBeChanged": lightcode });
        // forza un timeout rapido per vedere subito l'aggiornamento
        setTimeout(LightTimer, 50);
    });

    setTimeout(LightTimer, 100);
});

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

		// cambia pagina se errata, oppure resetta il modo
		// inoltre aggiorna il bottone play/pausa/continua
		switch (data.PlaMode) {
			case MODE.MODE_IDLE:
				// aggiorna l'interfaccia. Evita refresh continui delle proprietà
			    if (oldMode != MODE.MODE_IDLE) {
			        // nascondi bottone play + abort
			        $("#play-button").hide();
			        $("#abort").hide();
			        // mostra tutti i valori
			        $("#off_in_run").css("display", "block");
			        oldMode = MODE.MODE_IDLE;
			    }
				break;
			case MODE.MODE_RUN:
				// predisponi bottone pausa o continua, usa un valore speciale per gestire
				//   le due condizioni pausa/continua
				if (data.bPaused && oldMode != MODE.MODE_RUN+100) {
					$("#play-button").html('<button id="play" class="small" onclick="play()"><i class="fa fa-rotate-right fa-lg"></i></button>');
					// non mostrare alcuni valori in RUN
					$("#off_in_run").css("display", "none");
					oldMode = MODE.MODE_RUN+100;
				}
				else if (!data.bPaused && oldMode != MODE.MODE_RUN) {
					$("#play-button").html('<button id="play" class="small" onclick="pause()"><i class="fa fa-pause fa-lg"></i></button>');
					// non mostrare alcuni valori in RUN
					$("#off_in_run").css("display", "none");
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
		
		// dati cambiati, accelera il campionamento
		setTimeout(AjaxTimer, 100);
	});
}

function LightTimer() {
	$.getJSON("/planetario.fcgi?luci", function (data, status) {
		strdata = JSON.stringify(data);
		if (strdata == lightdata_old) {
			// se non ci sono cambiamenti, usa timeout più lungo (diverso da AjaxTimer,
			//	 così tendono a non sovrapporsi)
			setTimeout(LightTimer, 235);
			return;
		}
		lightdata_old = strdata;

		// non aggiornare la lista dei valori finché è visualizzato lo slider (copre)
		if (modal.style.display != 'block') {
			// aggiorna tutti i campi
			$("#pianeti").html(data.pianeti);
			$("#declinazione").html(data.declinazione);
			$("#p_cardinali").html(data.p_cardinali);
			//$("#luce_sala").html(data.luce_sala);
			$("#cerchio_polare").html(data.cerchio_polare);
			$("#alba").html(data.alba);
			$("#giorno").html(data.giorno);
			$("#tramonto").html(data.tramonto);
			$("#sole").html(data.sole);
			$("#luna").html(data.luna);
			$("#stelle").html(data.stelle);
			//$("#cerchio_orario").html(data.cerchio_orario);

			$("#supernova").html(data.supernova);
			$("#p_nebula").html(data.p_nebula);
			/*$("#nuove_s").html(data.nuove_s);
			$("#a_libero").html(data.a_libero);*/
		}
		else {
            // visualizzati slider e selector
		    // non aggiornare durante lo sliding
		    if (!sliding) {
		        var sel_var = $("#slider").attr("title");
		        var value = data[sel_var];

		        $("#slider").slider("value", value);
		        // metti anche il valore
		        $("#name_value").html(sel_var + " = " + value);
		        // ... e imposta il selector
		        var selection;
		        if (value == 0)
		            selection = 0;
		        else if (value < (light_low + light_med) / 2)
		            selection = 1;
		        else if (value < (light_med + light_high) / 2)
		            selection = 2;
		        else
		            selection = 3;

		        $("#selector")[0].selectedIndex = selection;
            }
		}

		// dati cambiati, accelera il campionamento
		setTimeout(LightTimer, 100);
	});
}

// configure and show slider for analogue lights
function show(light, id) {
	lightcode = light;
	var modal = document.getElementById("modal");

	switch (light) {
		case LIGHT_ID.LIGHT_ALBA:
		case LIGHT_ID.LIGHT_GIORNO:
	    case LIGHT_ID.LIGHT_TRAMONTO:
	        light_low = 5;
	        light_med = 100;
			light_high = 255;
			break;
		case LIGHT_ID.LIGHT_SOLE:
		    light_low = 50;
		    light_med = 200;
		    light_high = 4000;
			break;
		case LIGHT_ID.LIGHT_STELLE:
		    light_low = 30;
		    light_med = 300;
		    light_high = 900;
			break;
		case LIGHT_ID.LIGHT_LUNA:
		case LIGHT_ID.LIGHT_CERCHIOORARIO:
		case LIGHT_ID.LIGHT_SUPERNOVA:
		case LIGHT_ID.LIGHT_PNEBULA:
		case LIGHT_ID.LIGHT_NUOVES:
		case LIGHT_ID.LIGHT_ALIBERO:
		    light_low = 50;
		    light_med = 200;
		    light_high = 4095;
			break;
	}

	$("#name_value").html(id);	// riaggiornato in LightTimer() col valore
	$("#slider").prop("title", id);
	$("#slider").slider("option", "min", 0);
	$("#slider").slider("option", "max", light_high);

	modal.style.display = 'block';
	$("#settings").css("display", "none");
}

// Toggle digital lights
function toggle(code, id) {
	var value = $("#" + id).html();
	var new_value = (value == "0") ? 1:0;
	$.post("/planetario.fcgi", {"lightLevel": new_value, "lightToBeChanged": code});
	// forza un timeout rapido per vedere subito l'aggiornamento
	setTimeout(LightTimer, 50);
}

// Pause
function pause() {
	$.post("/planetario.fcgi", {"bSuspend": 1});
}

// Play
function play() {
	$.post("/planetario.fcgi", {"bContinue": 1});
}

// go back to correct page
function goback() {
	if (PlaMode == MODE.MODE_RUN)
		window.location.replace("simulation.html");
	else
		window.location.replace("index.html");
}
