/******************************************

precdecl.js

@author: Marco Visentin

******************************************/
var oldMode;
var PlaMode;
var latitude, annoPrecessione;
var beepDone = false;


function AjaxTimer() {
    $.getJSON("/planetario.fcgi?mainset", function (data, status) {
        PlaMode = data.PlaMode;

        // visualizza/resetta errori
        checkError(data.idError, "");

        // aggiorna precessione
        annoPrecessione = data.annoPrecessione;
        $("#precessione").html(annoPrecessione.toFixed(0));

        // cambia pagina se errata, oppure resetta il modo
        // inoltre aggiorna il bottone play/pausa/continua
        switch (data.PlaMode) {
            case MODE.MODE_IDLE:
                // aggiorna l'interfaccia. Evita refresh continui delle proprietà
                if (oldMode != MODE.MODE_IDLE) {
                    // nascondi bottone play + abort
                    $("#play-button").hide();
                    $("#abort").hide();
                    oldMode = MODE.MODE_IDLE;
                }
                break;
            case MODE.MODE_RUN:
                // predisponi bottone pausa o continua, usa un valore speciale per gestire
                //   le due condizioni pausa/continua
                if (data.bPaused && oldMode != MODE.MODE_RUN + 100) {
                    $("#play-button").html('<button id="play" class="small" onclick="play()"><i class="fa fa-rotate-right fa-lg"></i></button>');
                    // non mostrare alcuni valori in RUN
                    $("#off_in_run").css("display", "none");
                    oldMode = MODE.MODE_RUN + 100;
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

        // programma il prossimo timeout
        setTimeout(AjaxTimer, 250);
		
		// leggi struttura AUXILIARY per il setpoint di latitudine
		$.getJSON("/planetario.fcgi?auxiliary", function (data, status) {
			// aggiorna variabile e valore a schermo
			latitude = data.latitude;
			$("#declinazione").html(latitude.toFixed(2) + "°");
		});

    });
}

//Decl
function decl(incr) {
    $.post("/planetario.fcgi", { "latitude": latitude + incr});
}

//Prec
function prec(incr) {
    $.post("/planetario.fcgi", { "annoPrecessione": annoPrecessione + incr });
}

// Pause
function pause() {
    $.post("/planetario.fcgi", { "bSuspend": 1 });
}

// Play
function play() {
    $.post("/planetario.fcgi", { "bContinue": 1 });
}

// go back to correct page
function goback() {
    if (PlaMode == MODE.MODE_RUN)
        window.location.replace("simulation.html");
    else
        window.location.replace("index.html");
}
