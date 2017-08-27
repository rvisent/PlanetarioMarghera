/******************************************

common.js

@author: Marco Visentin

******************************************/

// disabilita caching del browser
// altrimenti alcuni browser non aggiornano i dati
$(document).ready(function () {
	$.ajaxSetup({ cache: false });
	// prima chiamata di aggiornamento, dipende dalla pagina
	AjaxTimer();
});

window.onload = function () {
	$("input").attr("autocomplete", "off");
	// appendi dummy space per consentire di nascondere la barra di titolo
	$("body").append('<div style="width: 100%; height: 200px; margin-bottom: 0;"></div>');
	// posiziona in cima
	location.hash = "#first";
}

//Strutture
const MODE = {
	MODE_OFFLINE: 0,
	MODE_IDLE: 1,
	MODE_SETPLANETS: 2,
	MODE_RUN: 3,
	MODE_ZERO: 4,
	MODE_SAFEPOS: 5,
	MODE_TEST: 6,
	MODE_SHUTDOWN: 7,
	MODE_SAVE: 8,
	MODE_DEFAULT: 9
}

const SETMODE = {
	SET_SOLE: 0,
	SET_LUNA: 1,
	SET_MERCURIO: 2,
	SET_VENERE: 3,
	SET_MARTE: 4,
	SET_GIOVE: 5,
	SET_SATURNO: 6
}

const SUBMODE = {
	SUB_NONE: 0,
	AZZERAMENTO: 1,
	POSIZIONAMENTO_INIZ: 2,
	PRONTO_POMERIGGIO: 3,
	RUN_POMERIGGIO: 4,
	RUN_TRAMONTO: 5,
	PRONTO_NOTTE: 6,
	RUN_NOTTE: 7,
	PRONTO_ALBA: 8,
	RUN_ALBA: 9,
	POSIZIONAMENTO_FIN: 10,
	PRONTO_TERMINARE: 11
}

const LIGHT_ID = {
	LIGHT_NONE: 0,
	LIGHT_SOLE: 1,
	LIGHT_LUNA: 2,
	LIGHT_STELLE: 3,
	LIGHT_CERCHIOORARIO: 4,
	LIGHT_SUPERNOVA: 5,
	LIGHT_PNEBULA: 6,
	LIGHT_NUOVES: 7,
	LIGHT_ALIBERO: 8,
	LIGHT_ALBA: 9,
	LIGHT_GIORNO: 10,
	LIGHT_TRAMONTO: 11,
	LIGHT_PIANETI: 12,
	LIGHT_DECLINAZIONE: 13,
	LIGHT_PCARDINALI: 14,
	LIGHT_LUCESALA: 15,
	LIGHT_CERCHIOPOLARE: 16,
	LIGHT_DLIBERO1: 17,
	LIGHT_DLIBERO2: 18,
	LIGHT_DLIBERO3: 19
}

const modeString = [ "OFFLINE", "IDLE", "SETPLANETS", "RUN", "AZZERAMENTO", "POSIZ.SICUR", "TEST", "SPEGNIMENTO",
					"SALVATAGGIO", "CARICAMENTO DATI" ];

const settingString = [ "SOLE", "LUNA", "MERCURIO", "VENERE", "MARTE", "GIOVE", "SATURNO" ];

const submodeString = [ "SUB_NONE", "AZZERAMENTO", "POSIZ. INIZIALE", "PRONTO POMERIGGIO", "RUN_POMERIGGIO",
					"RUN TRAMONTO", "PRONTO NOTTE", "RUN NOTTE", "PRONTO ALBA", "RUN ALBA", "POSIZ. FINALE",
					"PRONTO TERMINARE" ];

const ERROR_LIST = ["", "ABORTED", "PLAZERO:ZEROENC", "PLAZERO:ZEROENC2", "PLAMAIN:NOTZEROED", "PLASICUR:GOTOSICUR",
					"SETTINGS:GOTOENC", "SETTINGS:OUTOFRANGE", "RUN:GOTOENC", "TEST:BADMOTORID", "TEST:ZEROENC",
					"TEST:GOTOENC", "PARAMS:DEFAULTED", "PARAMS:WRITE"];

function getModeString(mode) {
	if (mode < 0 || mode > MODE.MODE_DEFAULT)
		return "*INVALID*";
	
	return modeString[mode];
}

function getSettingString(mode) {
	if (mode < 0 || mode > SETMODE.SET_SATURNO)
		return "*INVALID*";
	
	return settingString[mode];
}

function getSubmodeString(mode, perc) {
	mode &= 0x1F;
	if (mode < 0 || mode > SUBMODE.PRONTO_TERMINARE)
	    return "*INVALID*";

    // appendi percentuale per i modi non statici della simulazione
	if (typeof perc != 'undefined' && (mode == 2 || mode == 4 || mode == 5 || mode == 7 || mode == 9 || mode == 10))
	    return submodeString[mode] + ' (' + perc + '%)';

	return submodeString[mode];
}


//Abort
function abort() {
    if (confirm("Confermi l'uscita?"))
        $.post("/planetario.fcgi", { "bAbort": 1 });
}

function checkError(eCode, pageToGo) {
	if (eCode != 0) {
		$.post("/planetario.fcgi", {"bResetError": 1});
		if (eCode != 1) {
			// Manual abort doesn't require a confirmation
			alert("Errore " + eCode + "\n" + ERROR_LIST[eCode]);
		}
		else {
			if (pageToGo != "") {
				// abort manuale, vai alla pagina specificata
				pageToGo_=pageToGo;
				setTimeout(delayedGo, 200);
			}
		}
	}
}

pageToGo_ = "";

function exitGo(pageToGo) {
	abort();
	// cambia pagina con ritardo, altrimenti abort() fallisce in Firefox
	pageToGo_ = pageToGo;
	setTimeout(delayedGo, 200);
}

function delayedGo() {
	window.location.replace(pageToGo_);
}

function delayedReload() {
    setTimeout(function () { window.location.reload() }, 200);
}

function PlaySound(soundObj) {
    try {
        var sound = document.getElementById(soundObj);
        if (sound) {
            sound.play();
        }
    }
    catch (err) { }
}
