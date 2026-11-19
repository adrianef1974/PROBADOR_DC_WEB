
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>

// ==========================================
// MAPA DE GPIO - ESP32 DEVKIT V1
// ==========================================

// Bus I2C compartido: pantalla SSD1307 + ADS1115
#define PIN_I2C_SDA       21
#define PIN_I2C_SCL       22

// Bus SPI reservado para microSD UNIBOT
#define PIN_SD_CS          4
#define PIN_SD_SCK        18
#define PIN_SD_MISO       19
#define PIN_SD_MOSI       23

// Pulsadores: todos conectados entre GPIO y GND
#define PIN_BTN_ARRIBA    13
#define PIN_BTN_ABAJO     14
#define PIN_BTN_IZQUIERDA 16
#define PIN_BTN_DERECHA   17
#define PIN_BTN_ENTER     25
#define PIN_BTN_STOP      26
#define PIN_BTN_START     27

// Etapa de potencia totem pole:
// LOW = ACTIVADA / HIGH = DESACTIVADA
#define PIN_TOTEM_POLE    32

// Entradas ADS1115 (direccion I2C prevista: 0x48)
// A0 = tension de fuente (divisor pendiente de dimensionar)
// A1 = reserva
// A2 = extremo del shunt hacia SOURCE
// A3 = extremo del shunt hacia GND
// Shunt: 0,1 ohm / 10 W

// GPIO libres reservados para futuras funciones
#define PIN_RESERVA_1      5
#define PIN_RESERVA_2     33

// Lista de pulsadores
const uint8_t pinesPulsadores[] = {
  PIN_BTN_ARRIBA, PIN_BTN_ABAJO, PIN_BTN_IZQUIERDA,
  PIN_BTN_DERECHA, PIN_BTN_ENTER, PIN_BTN_STOP, PIN_BTN_START
};


// ==========================================
// CONFIGURACION WIFI
// ==========================================

const char* ssid = "PROBADOR_DC";
const char* password = "12345678";

const char* usuarioWeb = "admin";
const char* claveWeb = "1813";

WebServer server(80);

// ==========================================
// PAGINA WEB
// ==========================================

const char pagina[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PROBADOR DC</title>

<style>
* {box-sizing:border-box;}

body {
 margin:0;
 background:#080d13;
 color:#dce8ef;
 font-family:Arial,sans-serif;
}

header {
 background:#111d29;
 padding:18px;
 text-align:center;
 color:#00e5ff;
 font-size:22px;
 font-weight:bold;
}

nav {
 display:flex;
 background:#101a25;
}

nav button {
 flex:1;
 padding:14px 3px;
 background:#172535;
 color:white;
 border:1px solid #26394a;
 font-size:13px;
 cursor:pointer;
}

.pantalla {display:none;padding:12px;}
.activa {display:block;}

.panel {
 background:#101a25;
 border:1px solid #26394a;
 border-radius:9px;
 padding:15px;
 margin-bottom:12px;
}

.titulo {
 color:#91a9bc;
 font-size:14px;
}

.valor {
 font-size:38px;
 font-weight:bold;
 margin:8px 0;
}

.voltaje {color:#00ff91;}
.corriente {color:#00d9ff;}

.grafico {
 width:100%;
 height:190px;
 display:block;
 background:#050b10;
 border:1px solid #26394a;
 border-radius:5px;
}

.estado {
 color:#00ff91;
 font-weight:bold;
 font-size:18px;
}

.botones {
 display:grid;
 grid-template-columns:1fr 1fr;
 gap:10px;
}

.botones button {
 padding:17px 5px;
 border:0;
 border-radius:6px;
 color:white;
 font-weight:bold;
 font-size:15px;
 cursor:pointer;
}

.inicio {background:#087c49;}
.reset {background:#265c9b;}
.fin {background:#a32c36;}
.descarga {background:#087c9b;}
.borrar {background:#a32c36;}

button:disabled {
 background:#39434c !important;
 color:#89939b !important;
 opacity:0.65;
}

h3 {
 color:#00d9ff;
 margin-top:0;
}

.tabla {
 width:100%;
 border-collapse:collapse;
 font-size:13px;
 min-width:420px;
}

.tabla th {
 background:#183044;
 color:#00d9ff;
 padding:12px 7px;
}

.tabla td {
 padding:11px 7px;
 border-bottom:1px solid #26394a;
 text-align:center;
}

.tabla tr:nth-child(even) {background:#152230;}

.aprobado {color:#00ff91;font-weight:bold;}
.rechazado {color:#ff5555;font-weight:bold;}

.contenedorTabla {overflow-x:auto;}

.contadores {
 display:flex;
 justify-content:space-around;
 text-align:center;
 gap:5px;
}

.contadores strong {
 display:block;
 font-size:22px;
 color:#00e5ff;
 margin-top:7px;
}

/* ==========================================
   CONFIGURACION
========================================== */

.seccionTitulo {
 color:#00e5ff;
 font-size:17px;
 font-weight:bold;
 border-bottom:1px solid #294354;
 padding-bottom:10px;
 margin-bottom:14px;
}

.itemHabilitacion {
 display:flex;
 align-items:center;
 justify-content:space-between;
 gap:12px;
 padding:12px 0;
 border-bottom:1px solid #26394a;
}

.itemHabilitacion:last-child {border-bottom:0;}

.nombreHabilitacion {
 font-size:14px;
 font-weight:bold;
}

.estadoHabilitacion {
 font-size:11px;
 color:#91a9bc;
 margin-top:4px;
}

.switch {
 position:relative;
 display:inline-block;
 width:52px;
 height:28px;
 flex-shrink:0;
}

.switch input {
 opacity:0;
 width:0;
 height:0;
}

.slider {
 position:absolute;
 cursor:pointer;
 inset:0;
 background:#39434c;
 border-radius:30px;
 transition:.2s;
}

.slider:before {
 position:absolute;
 content:"";
 height:20px;
 width:20px;
 left:4px;
 bottom:4px;
 background:white;
 border-radius:50%;
 transition:.2s;
}

.switch input:checked + .slider {
 background:#087c49;
}

.switch input:checked + .slider:before {
 transform:translateX(24px);
}

.campo {
 margin-bottom:16px;
}

.campo label {
 display:block;
 color:#b9cbd7;
 font-size:13px;
 margin-bottom:7px;
}

.campo input[type=number] {
 width:100%;
 padding:12px;
 background:#07111a;
 color:#00e5ff;
 border:1px solid #345064;
 border-radius:5px;
 font-size:17px;
 outline:none;
}

.campo input:focus {
 border-color:#00d9ff;
}

.campo small {
 display:block;
 color:#71899a;
 font-size:11px;
 margin-top:5px;
}

.valorSlider {
 color:#00e5ff;
 font-weight:bold;
 font-size:17px;
 float:right;
}

input[type=range] {
 width:100%;
 accent-color:#00d9ff;
 margin-top:8px;
}

.botonesConfig {
 display:grid;
 grid-template-columns:1fr 1fr;
 gap:10px;
}

.botonesConfig button {
 padding:15px 5px;
 border:0;
 border-radius:6px;
 color:white;
 font-weight:bold;
 font-size:13px;
 cursor:pointer;
}

.guardar {background:#087c49;}
.restaurar {background:#9b5722;}

.avisoConfig {
 color:#91a9bc;
 font-size:12px;
 line-height:1.5;
}

@media(max-width:420px) {
 nav button {font-size:11px;}
 .valor {font-size:32px;}
 .botonesConfig button {font-size:11px;}
}
</style>
</head>

<body>

<header>PROBADOR DC</header>

<nav>
<button onclick="mostrar(0)">INICIO</button>
<button onclick="mostrar(1)">PRUEBAS</button>
<button onclick="entrarConfig()">CONFIGURACION</button>
</nav>

<!-- ================================== -->
<!-- PANTALLA 1 - INICIO -->
<!-- ================================== -->

<div class="pantalla activa" id="p0">

<div class="panel">
<div class="titulo">TENSION DE FUENTE</div>
<div class="valor voltaje">
<span id="v">0.00</span> V
</div>
<canvas class="grafico" id="graficoV"></canvas>
</div>

<div class="panel">
<div class="titulo">CORRIENTE DE PRUEBA</div>
<div class="valor corriente">
<span id="a">0.000</span> A
</div>
<canvas class="grafico" id="graficoA"></canvas>
</div>

<div class="panel">
<div class="titulo">ESTADO DE LA PRUEBA</div>
<p class="estado" id="estado">ESPERANDO INICIO</p>
</div>

<div class="botones">
<button class="inicio" onclick="accion('INICIO')">INICIO</button>
<button class="reset" onclick="accion('RESET')">RESET</button>
<button class="fin" onclick="accion('FIN')">FIN</button>
</div>

</div>

<!-- ================================== -->
<!-- PANTALLA 2 - HISTORIAL -->
<!-- ================================== -->

<div class="pantalla" id="p1">

<div class="panel">
<h3>HISTORIAL DE PRUEBAS</h3>

<div class="contadores">
<div>TOTAL<strong id="total">0</strong></div>
<div>APROBADOS<strong id="aprobados">0</strong></div>
<div>RECHAZADOS<strong id="rechazados">0</strong></div>
</div>
</div>

<div class="panel">
<div class="contenedorTabla">
<table class="tabla">
<thead>
<tr>
<th>N° PRUEBA</th>
<th>TENSION (V)</th>
<th>CORRIENTE (A)</th>
<th>RESULTADO</th>
</tr>
</thead>
<tbody id="historial"></tbody>
</table>
</div>
</div>

<div class="botones">
<button class="descarga" onclick="descargar()">
DESCARGAR EXCEL
</button>

<button class="borrar" id="btnBorrar"
onclick="borrarHistorial()" disabled>
BORRAR HISTORIAL
</button>
</div>

<p id="mensaje" style="color:#91a9bc;font-size:13px;text-align:center">
Primero descargue el historial para habilitar BORRAR.
</p>

</div>

<!-- ================================== -->
<!-- PANTALLA 3 - CONFIGURACION -->
<!-- ================================== -->

<div class="pantalla" id="p2">

<!-- HABILITACIONES -->

<div class="panel">

<div class="seccionTitulo">1. HABILITACIONES</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">ENTRADA DE VOLTAJE</div>
<div class="estadoHabilitacion" id="txtVoltaje">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habVoltaje">
<span class="slider"></span>
</label>
</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">SALIDA PWM</div>
<div class="estadoHabilitacion" id="txtPWM">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habPWM">
<span class="slider"></span>
</label>
</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">NTC</div>
<div class="estadoHabilitacion" id="txtNTC">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habNTC">
<span class="slider"></span>
</label>
</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">CORRIENTE</div>
<div class="estadoHabilitacion" id="txtCorriente">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habCorriente">
<span class="slider"></span>
</label>
</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">PROTECCION POR TEMPERATURA</div>
<div class="estadoHabilitacion" id="txtProteccion">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habProteccion">
<span class="slider"></span>
</label>
</div>

<div class="itemHabilitacion">
<div>
<div class="nombreHabilitacion">REGISTRO DE PRUEBAS</div>
<div class="estadoHabilitacion" id="txtRegistro">DESHABILITADO</div>
</div>
<label class="switch">
<input type="checkbox" id="habRegistro">
<span class="slider"></span>
</label>
</div>

</div>

<!-- PARAMETROS -->

<div class="panel">

<div class="seccionTitulo">2. PARAMETROS CONFIGURABLES</div>

<h3>MODO DE PRUEBA</h3>
<div class="campo">
<label>SELECCIONAR MODO</label>
<select id="modoPrueba" style="width:100%;padding:12px;background:#07111a;color:#00e5ff;border:1px solid #345064;border-radius:5px;font-size:16px">
<option value="AUTOMATICO">AUTOMATICO</option>
<option value="MANUAL">MANUAL</option>
<option value="CICLICO">CICLICO</option>
</select>
<small>AUTOMATICO queda seleccionado por defecto.</small>
</div>
<div id="parametrosCiclicos">
<h3>TIEMPOS DEL MODO CICLICO</h3>
<div class="campo">
<label>TIEMPO DE CARGA (segundos)</label>
<input type="number" id="tiempoCargaCiclico" min="1" step="1" placeholder="Ingresar valor">
</div>
<div class="campo">
<label>TIEMPO SIN CARGA (segundos)</label>
<input type="number" id="tiempoSinCargaCiclico" min="1" step="1" placeholder="Ingresar valor">
</div>
<small>Durante la carga se respeta la corriente maxima configurada. Durante la pausa, la carga se desconecta.</small>
</div>
<h3>LIMITES DE TENSION</h3>

<div class="campo">
<label>TENSION MINIMA (V)</label>
<input type="number" id="tensionMin" step="0.01" placeholder="Ingresar valor">
</div>

<div class="campo">
<label>TENSION MAXIMA (V)</label>
<input type="number" id="tensionMax" step="0.01" placeholder="Ingresar valor">
</div>

<h3>LIMITES DE CORRIENTE</h3>

<div class="campo">
<label>CORRIENTE MINIMA (A)</label>
<input type="number" id="corrienteMin" step="0.001" placeholder="Ingresar valor">
</div>

<div class="campo">
<label>CORRIENTE MAXIMA (A)</label>
<input type="number" id="corrienteMax" step="0.001" placeholder="Ingresar valor">
</div>

<h3>TOLERANCIA DE TENSION</h3>

<div class="campo">
<label>
TOLERANCIA (%)
<span class="valorSlider" id="valorTolerancia">0 %</span>
</label>
<input type="range" id="tolerancia"
min="0" max="30" step="1" value="0"
oninput="actualizarSlider('tolerancia','valorTolerancia',' %')">
<small>Una sola escala porcentual para la variacion de tension.</small>
</div>

<h3>TEMPERATURA</h3>

<div class="campo">
<label>TEMPERATURA MAXIMA (°C)</label>
<input type="number" id="temperaturaMax" step="1" placeholder="Ingresar valor">
</div>

<h3>TIEMPOS</h3>

<div class="campo">
<label>TIEMPO DE PRUEBA (segundos)</label>
<input type="number" id="tiempoPruebaConfig" min="0" step="1" placeholder="Ingresar valor">
</div>

<div class="campo">
<label>TIEMPO DE ESTABILIZACION (segundos)</label>
<input type="number" id="tiempoEstabilizacion" min="0" step="1" placeholder="Ingresar valor">
</div>

<h3>SALIDA PWM</h3>

<div class="campo">
<label>
PORCENTAJE PWM (%)
<span class="valorSlider" id="valorPWM">0 %</span>
</label>
<input type="range" id="porcentajePWM"
min="0" max="100" step="1" value="0"
oninput="actualizarSlider('porcentajePWM','valorPWM',' %')">
</div>

<div class="campo">
<label>FRECUENCIA PWM (Hz)</label>
<input type="number" id="frecuenciaPWM" min="0" step="1" placeholder="Ingresar valor">
</div>

</div>

<!-- ADMINISTRACION -->

<div class="panel">

<div class="seccionTitulo">3. ADMINISTRACION</div>

<div class="botonesConfig">
<button class="guardar" onclick="guardarConfig()">
GUARDAR CONFIGURACION
</button>

<button class="restaurar" onclick="restaurarConfig()">
RESTAURAR VALORES DE FABRICA
</button>
</div>

<p class="avisoConfig" id="mensajeConfig">
Los cambios se aplican al guardar la configuracion.
</p>

</div>

</div>

<script>

// ==========================================
// NAVEGACION
// ==========================================

function mostrar(n) {
 document.querySelectorAll('.pantalla').forEach(
  p=>p.classList.remove('activa')
 );
 document.getElementById('p'+n).classList.add('activa');
}

// ==========================================
// ACCESO A CONFIGURACION
// ==========================================

async function entrarConfig() {

 let usuario=prompt("USUARIO DE CONFIGURACION");
 if(usuario===null)return;

 let clave=prompt("CLAVE DE CONFIGURACION");
 if(clave===null)return;

 let autorizacion="Basic "+btoa(usuario+":"+clave);

 try {
  let respuesta=await fetch('/config',{
   headers:{'Authorization':autorizacion}
  });

  if(respuesta.ok){
   cargarConfig();
   mostrar(2);
  } else {
   alert("USUARIO O CLAVE INCORRECTOS");
  }

 } catch(e) {
  alert("ERROR DE CONEXION");
 }
}

// ==========================================
// GRAFICAS CON ESCALA DINAMICA
// ==========================================

const MAX_PUNTOS=180;
let datosV=Array(MAX_PUNTOS).fill(0);
let datosA=Array(MAX_PUNTOS).fill(0);

function leerConfigActual(){
 let guardada=localStorage.getItem('configDC');
 if(!guardada)return null;
 try{return JSON.parse(guardada);}catch(e){return null;}
}

function valorConfig(id){
 let c=leerConfigActual();
 if(!c || !c.valores || c.valores[id]===undefined || c.valores[id]==='')return null;
 let n=Number(c.valores[id]);
 return Number.isFinite(n)?n:null;
}

function habilitado(id){
 let c=leerConfigActual();
 return !!(c && c.habilitaciones && c.habilitaciones[id]===true);
}

function escalaVoltaje(){
 let max=valorConfig('tensionMax');
 let tol=0;
 let c=leerConfigActual();
 if(c && Number.isFinite(Number(c.tolerancia)))tol=Number(c.tolerancia);
 if(max===null || max<=0)return 15;
 return Math.max(1,max*(1+tol/100));
}

function escalaCorriente(){
 let max=valorConfig('corrienteMax');
 return max!==null && max>0?max:2;
}

function dibujarGrafico(id,datos,maximo,color,unidad,referencia){
 let canvas=document.getElementById(id);
 let ctx=canvas.getContext('2d');
 let ancho=canvas.clientWidth, alto=canvas.clientHeight;
 let dpr=window.devicePixelRatio||1;
 canvas.width=ancho*dpr; canvas.height=alto*dpr;
 ctx.setTransform(dpr,0,0,dpr,0,0);
 ctx.clearRect(0,0,ancho,alto);
 let izquierda=43,derecha=10,arriba=12,abajo=25;
 let w=ancho-izquierda-derecha,h=alto-arriba-abajo;
 ctx.fillStyle='#050b10';ctx.fillRect(0,0,ancho,alto);
 ctx.font='10px Arial';ctx.textAlign='right';
 for(let i=0;i<=5;i++){
  let y=arriba+(h/5)*i, v=maximo-(maximo/5)*i;
  ctx.beginPath();ctx.strokeStyle='#28404e';ctx.lineWidth=1;
  ctx.moveTo(izquierda,y);ctx.lineTo(izquierda+w,y);ctx.stroke();
  ctx.fillStyle='#91a9bc';ctx.fillText(v.toFixed(1),izquierda-5,y+3);
 }
 for(let i=0;i<=6;i++){
  let x=izquierda+(w/6)*i;
  ctx.beginPath();ctx.strokeStyle='#1b3442';ctx.moveTo(x,arriba);ctx.lineTo(x,arriba+h);ctx.stroke();
 }
 if(referencia!==null && referencia!==undefined && referencia>=0 && referencia<=maximo){
  let ry=arriba+h-(referencia/maximo)*h;
  ctx.beginPath();ctx.setLineDash([4,4]);ctx.strokeStyle='#e5b94b';
  ctx.moveTo(izquierda,ry);ctx.lineTo(izquierda+w,ry);ctx.stroke();ctx.setLineDash([]);
 }
 ctx.beginPath();ctx.strokeStyle='#708493';ctx.lineWidth=1.5;
 ctx.moveTo(izquierda,arriba+h);ctx.lineTo(izquierda+w,arriba+h);ctx.stroke();
 ctx.fillStyle='#91a9bc';ctx.textAlign='center';ctx.fillText(unidad,izquierda+w/2,alto-5);
 ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=2;ctx.shadowBlur=7;ctx.shadowColor=color;
 datos.forEach((v,i)=>{
  let x=izquierda+(i/(MAX_PUNTOS-1))*w;
  let y=arriba+h-(v/maximo)*h;
  y=Math.max(arriba,Math.min(arriba+h,y));
  if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
 });
 ctx.stroke();ctx.shadowBlur=0;
 let ultimo=datos[datos.length-1];
 let py=arriba+h-(ultimo/maximo)*h;
 py=Math.max(arriba,Math.min(arriba+h,py));
 ctx.beginPath();ctx.fillStyle=color;ctx.arc(izquierda+w,py,3,0,Math.PI*2);ctx.fill();
}

// ==========================================
// SIMULACION DE LA SECUENCIA DE PRUEBA
// NOTA: ESTA VERSION WEB TODAVIA NO LEE
// SENSORES NI ACCIONA EL PWM FISICO.
// ==========================================

let tensionActual=0,corrienteActual=0,corrientePico=0;
let pruebaIniciada=false;
let etapaPrueba='ESPERANDO';
let inicioEtapa=0,inicioPrueba=0;
let pwmActual=0;
let tensionInicial=0,tensionCarga=0,tensionRecuperada=0;
let cortePorCorriente=false;
let etapaCiclica='CARGA';
let modoEnCurso='AUTOMATICO';

function actualizarEstado(txt){
 document.getElementById('estado').innerText=txt;
}

function iniciarPrueba(){
 if(pruebaIniciada)return;
 let configInicio=leerConfigActual();
 modoEnCurso=(configInicio && configInicio.modoPrueba)?configInicio.modoPrueba:'AUTOMATICO';
 if(modoEnCurso==='MANUAL'){
  actualizarEstado('MODO MANUAL: ESPERANDO BOTON FISICO. GPIO PENDIENTE DE DEFINIR');
  return;
 }
 if(modoEnCurso==='CICLICO'){
  let tCarga=valorConfig('tiempoCargaCiclico');
  let tPausa=valorConfig('tiempoSinCargaCiclico');
  if(tCarga===null || tCarga<=0 || tPausa===null || tPausa<=0){
   actualizarEstado('CONFIGURE TIEMPO DE CARGA Y TIEMPO SIN CARGA');
   return;
  }
 }
 let vMin=valorConfig('tensionMin'),vMax=valorConfig('tensionMax');
 // Fuente simulada de 12 V para probar la interfaz.
 const fuenteDemo=12.00;
 if(vMin!==null && fuenteDemo<vMin){
  actualizarEstado('FUENTE RECHAZADA: TENSION MENOR AL MINIMO');return;
 }
 if(vMax!==null && fuenteDemo>vMax){
  actualizarEstado('FUENTE RECHAZADA: TENSION MAYOR AL MAXIMO');return;
 }
 if(!habilitado('habPWM')){
  actualizarEstado('SALIDA PWM DESHABILITADA EN CONFIGURACION');return;
 }
 let duracion=valorConfig('tiempoPruebaConfig');
 let estabilizacion=valorConfig('tiempoEstabilizacion');
 if(modoEnCurso==='AUTOMATICO' && (duracion===null || duracion<=0 || estabilizacion===null || estabilizacion<0)){
  actualizarEstado('CONFIGURE TIEMPO DE PRUEBA Y ESTABILIZACION');return;
 }
 pruebaIniciada=true;
 cortePorCorriente=false;
 etapaCiclica='CARGA';
 pwmActual=0;
 tensionInicial=fuenteDemo;
 tensionCarga=0;
 tensionRecuperada=0;
 tensionActual=fuenteDemo;
 corrienteActual=0;
 etapaPrueba='CARGA';
 inicioPrueba=Date.now();
 inicioEtapa=inicioPrueba;
 actualizarEstado('TENSION EN VACIO INICIAL: '+tensionInicial.toFixed(2)+' V');
}

function finalizarPrueba(){
 pruebaIniciada=false;
 pwmActual=0;
 corrienteActual=0;
 tensionActual=tensionRecuperada||tensionActual;
 etapaPrueba='FINALIZADA';
 actualizarEstado('PRUEBA FINALIZADA - VACIO INICIAL '+tensionInicial.toFixed(2)+' V / CARGA '+tensionCarga.toFixed(2)+' V / ESTABILIZADA '+tensionRecuperada.toFixed(2)+' V');
 if(habilitado('habRegistro')){
  let h=JSON.parse(localStorage.getItem('historialDC')||'[]');
  h.push({
   numero:h.length+1,
   tension:tensionCarga.toFixed(2),
   corriente:corrientePico.toFixed(3),
   resultado:cortePorCorriente?'CORTE CORRIENTE':'FINALIZADA'
  });
  localStorage.setItem('historialDC',JSON.stringify(h));
  cargarHistorial();
 }
}

function actualizar(){
 let ahora=Date.now();
 if(pruebaIniciada && modoEnCurso==='CICLICO'){
  let tCarga=valorConfig('tiempoCargaCiclico')||1;
  let tPausa=valorConfig('tiempoSinCargaCiclico')||1;
  let transcurrido=(ahora-inicioEtapa)/1000;
  let configCiclo=leerConfigActual();
  let maxPWM=configCiclo?Number(configCiclo.porcentajePWM||0):0;
  let maxCorriente=valorConfig('corrienteMax');
  if(etapaCiclica==='CARGA'){
   pwmActual=maxPWM;
   corrienteActual=maxCorriente!==null && maxCorriente>0?maxCorriente:0;
   corrientePico=Math.max(corrientePico,corrienteActual);
   tensionActual=Math.max(0,12-(pwmActual/100)*1.2);
   tensionCarga=tensionActual;
   actualizarEstado('MODO CICLICO - CARGA');
   if(transcurrido>=tCarga){
    pwmActual=0;corrienteActual=0;etapaCiclica='SIN_CARGA';inicioEtapa=ahora;
    actualizarEstado('MODO CICLICO - SIN CARGA');
   }
  }else{
   pwmActual=0;corrienteActual=0;tensionActual=12;
   actualizarEstado('MODO CICLICO - SIN CARGA');
   if(transcurrido>=tPausa){
    etapaCiclica='CARGA';inicioEtapa=ahora;
    actualizarEstado('MODO CICLICO - CARGA');
   }
  }
 }else if(pruebaIniciada){
  let transcurrido=(ahora-inicioEtapa)/1000;
  let duracion=valorConfig('tiempoPruebaConfig')||1;
  let maxPWM=leerConfigActual()?Number(leerConfigActual().porcentajePWM||0):0;
  let maxCorriente=valorConfig('corrienteMax');
  if(etapaPrueba==='CARGA'){
   let progreso=Math.min(1,transcurrido/duracion);
   let pwmObjetivo=maxPWM*progreso;
   if(maxCorriente!==null && maxCorriente>0){
    // Modelo demostrativo: corriente proporcional al PWM.
    let corrienteObjetivo=(pwmObjetivo/100)*Math.max(maxCorriente,0.001)*1.15;
    if(corrienteObjetivo>=maxCorriente){
     pwmObjetivo=Math.min(pwmObjetivo,(maxCorriente/(Math.max(maxCorriente,0.001)*1.15))*100);
     cortePorCorriente=true;
    }
   }
   pwmActual=Math.min(maxPWM,pwmObjetivo);
   corrienteActual=(pwmActual/100)*(maxCorriente!==null && maxCorriente>0?maxCorriente*1.15:1.0);
   corrientePico=Math.max(corrientePico,corrienteActual);
   tensionActual=Math.max(0,12-(pwmActual/100)*1.2);
   tensionCarga=tensionActual;
   actualizarEstado('TENSION EN CARGA - PWM '+pwmActual.toFixed(1)+' %'+(cortePorCorriente?' - LIMITE DE CORRIENTE ALCANZADO':''));
   if(progreso>=1 || cortePorCorriente){
    pwmActual=0;corrienteActual=0;
    etapaPrueba='ESTABILIZACION';inicioEtapa=ahora;
    actualizarEstado('TENSION EN VACIO - ESTABILIZACION');
   }
  }else if(etapaPrueba==='ESTABILIZACION'){
   tensionActual=12-(0.15*Math.exp(-transcurrido/1.5));
   tensionRecuperada=tensionActual;
   corrienteActual=0;
   let tEst=valorConfig('tiempoEstabilizacion')||0;
   if(transcurrido>=tEst)finalizarPrueba();
  }
 }else if(!pruebaIniciada && etapaPrueba==='ESPERANDO'){
  tensionActual=0;corrienteActual=0;
 }
 document.getElementById('v').innerText=tensionActual.toFixed(2);
 document.getElementById('a').innerText=corrienteActual.toFixed(3);
 datosV.push(tensionActual);datosA.push(corrienteActual);
 datosV.shift();datosA.shift();
 let cMin=valorConfig('corrienteMin');
 dibujarGrafico('graficoV',datosV,escalaVoltaje(),'#00ff91','TIEMPO',null);
 dibujarGrafico('graficoA',datosA,escalaCorriente(),'#00d9ff','TIEMPO',cMin);
}

setInterval(actualizar,100);
actualizar();

// ==========================================
// ACCIONES DE PRUEBA
// ==========================================

function accion(nombre){
 if(nombre==='INICIO'){iniciarPrueba();return;}
 if(nombre==='RESET'){
  pruebaIniciada=false;etapaPrueba='ESPERANDO';pwmActual=0;
  tensionActual=0;corrienteActual=0;tensionInicial=0;
  tensionCarga=0;tensionRecuperada=0;cortePorCorriente=false;
  actualizarEstado('SISTEMA REINICIADO');return;
 }
 if(nombre==='FIN'){
  if(!pruebaIniciada){actualizarEstado('NO HAY PRUEBA EN CURSO');return;}
  pwmActual=0;corrienteActual=0;
  tensionRecuperada=tensionActual;
  finalizarPrueba();
 }
}

// ==========================================
// HISTORIAL
// ==========================================

function cargarHistorial(){

 let historial=JSON.parse(
  localStorage.getItem('historialDC')||'[]'
 );

 let tabla=document.getElementById('historial');

 tabla.innerHTML='';

 let aprobados=0;
 let rechazados=0;

 historial.forEach(r=>{

  if(r.resultado==='APROBADO')aprobados++;
  else rechazados++;

  let fila=document.createElement('tr');

  fila.innerHTML=`
  <td>${String(r.numero).padStart(3,'0')}</td>
  <td>${r.tension}</td>
  <td>${r.corriente}</td>
  <td class="${r.resultado==='APROBADO'?'aprobado':'rechazado'}">
  ${r.resultado}
  </td>`;

  tabla.appendChild(fila);
 });

 document.getElementById('total').innerText=historial.length;
 document.getElementById('aprobados').innerText=aprobados;
 document.getElementById('rechazados').innerText=rechazados;
}

// ==========================================
// DESCARGA CSV
// ==========================================

function descargar(){

 let historial=JSON.parse(
  localStorage.getItem('historialDC')||'[]'
 );

 if(historial.length===0){
  alert('NO HAY PRUEBAS PARA DESCARGAR');
  return;
 }

 let csv='N° PRUEBA;TENSION (V);CORRIENTE (A);RESULTADO\n';

 historial.forEach(r=>{
  csv+=`${r.numero};${r.tension};${r.corriente};${r.resultado}\n`;
 });

 let archivo=new Blob(
  ['\uFEFF'+csv],
  {type:'text/csv;charset=utf-8;'}
 );

 let url=URL.createObjectURL(archivo);

 let enlace=document.createElement('a');
 enlace.href=url;
 enlace.download='HISTORIAL_PROBADOR_DC.csv';

 document.body.appendChild(enlace);
 enlace.click();
 enlace.remove();

 setTimeout(()=>{
  URL.revokeObjectURL(url);

  localStorage.setItem('descargaDC','SI');
  actualizarBorrar();

 },1000);
}

// ==========================================
// BORRAR HISTORIAL
// ==========================================

function actualizarBorrar(){

 let habilitado=
 localStorage.getItem('descargaDC')==='SI';

 document.getElementById('btnBorrar').disabled=!habilitado;
}

function borrarHistorial(){

 if(localStorage.getItem('descargaDC')!=='SI'){
  alert('PRIMERO DESCARGUE EL HISTORIAL');
  return;
 }

 if(!confirm('¿DESEA BORRAR TODO EL HISTORIAL?'))return;

 localStorage.removeItem('historialDC');
 localStorage.removeItem('descargaDC');

 cargarHistorial();
 actualizarBorrar();

 document.getElementById('mensaje').innerText=
 'HISTORIAL BORRADO CORRECTAMENTE';
}

// ==========================================
// CONFIGURACION - DATOS
// ==========================================

const camposConfig=[
 'tensionMin',
 'tensionMax',
 'corrienteMin',
 'corrienteMax',
 'temperaturaMax',
 'tiempoPruebaConfig',
 'tiempoEstabilizacion',
 'frecuenciaPWM',
 'tiempoCargaCiclico',
 'tiempoSinCargaCiclico'
];

const habilitaciones=[
 ['habVoltaje','txtVoltaje'],
 ['habPWM','txtPWM'],
 ['habNTC','txtNTC'],
 ['habCorriente','txtCorriente'],
 ['habProteccion','txtProteccion'],
 ['habRegistro','txtRegistro']
];

// ==========================================
// ACTUALIZAR TEXTO DE INTERRUPTORES
// ==========================================

function actualizarEstadosHabilitaciones(){

 habilitaciones.forEach(par=>{

  let control=document.getElementById(par[0]);
  let texto=document.getElementById(par[1]);

  texto.innerText=control.checked?
   'HABILITADO':'DESHABILITADO';

  texto.style.color=control.checked?
   '#00ff91':'#91a9bc';
 });
}

habilitaciones.forEach(par=>{

 document.getElementById(par[0]).addEventListener(
  'change',actualizarEstadosHabilitaciones
 );

});

// ==========================================
// BARRAS DESLIZANTES
// ==========================================

function actualizarSlider(id,etiqueta,sufijo){

 let valor=document.getElementById(id).value;

 document.getElementById(etiqueta).innerText=
 valor+sufijo;
}

// ==========================================
// CARGAR CONFIGURACION
// ==========================================

function cargarConfig(){

 let guardada=localStorage.getItem('configDC');

 let config;

 if(guardada){
  try {
   config=JSON.parse(guardada);
  } catch(e) {
   config=null;
  }
 }

 if(!config){
  config={
   habilitaciones:{},
   valores:{},
   tolerancia:0,
   porcentajePWM:0
  };
 }

 habilitaciones.forEach(par=>{

  document.getElementById(par[0]).checked=
   config.habilitaciones[par[0]]===true;

 });

 camposConfig.forEach(id=>{

  document.getElementById(id).value=
   config.valores[id]!==undefined?
   config.valores[id]:'';

 });

 document.getElementById('tolerancia').value=
  config.tolerancia!==undefined?config.tolerancia:0;

 document.getElementById('modoPrueba').value=
  config.modoPrueba||'AUTOMATICO';
 actualizarVisibilidadCiclica();

 document.getElementById('porcentajePWM').value=
  config.porcentajePWM!==undefined?config.porcentajePWM:0;

 actualizarEstadosHabilitaciones();

 actualizarSlider('tolerancia','valorTolerancia',' %');
 actualizarSlider('porcentajePWM','valorPWM',' %');
}

// ==========================================
// GUARDAR CONFIGURACION
// ==========================================

function guardarConfig(){

 let config={
  habilitaciones:{},
  valores:{},
  tolerancia:Number(document.getElementById('tolerancia').value),
  porcentajePWM:Number(document.getElementById('porcentajePWM').value),
  modoPrueba:document.getElementById('modoPrueba').value
 };

 habilitaciones.forEach(par=>{

  config.habilitaciones[par[0]]=
   document.getElementById(par[0]).checked;

 });

 camposConfig.forEach(id=>{

  let valor=document.getElementById(id).value;

  config.valores[id]=
   valor===''?'':Number(valor);

 });

 // Comprobar limites cuando ambos estan ingresados

 let vMin=config.valores.tensionMin;
 let vMax=config.valores.tensionMax;

 if(vMin!=='' && vMax!=='' && vMin>vMax){
  alert('La tension minima no puede ser mayor que la maxima.');
  return;
 }

 let cMin=config.valores.corrienteMin;
 let cMax=config.valores.corrienteMax;

 if(cMin!=='' && cMax!=='' && cMin>cMax){
  alert('La corriente minima no puede ser mayor que la maxima.');
  return;
 }

 localStorage.setItem('configDC',JSON.stringify(config));

 document.getElementById('mensajeConfig').innerText=
  'CONFIGURACION GUARDADA CORRECTAMENTE.';

 document.getElementById('mensajeConfig').style.color=
  '#00ff91';
}

// ==========================================
// MODO DE PRUEBA
// ==========================================

function actualizarVisibilidadCiclica(){
 let modo=document.getElementById('modoPrueba').value;
 document.getElementById('parametrosCiclicos').style.display=
  modo==='CICLICO'?'block':'none';
}

document.getElementById('modoPrueba').addEventListener('change',actualizarVisibilidadCiclica);

// ==========================================
// RESTAURAR VALORES DE FABRICA
// ==========================================

function restaurarConfig(){

 if(!confirm(
  '¿DESEA RESTAURAR LA CONFIGURACION DE FABRICA?'
 ))return;

 localStorage.removeItem('configDC');

 cargarConfig();

 document.getElementById('mensajeConfig').innerText=
  'CONFIGURACION RESTAURADA. TODAS LAS HABILITACIONES ESTAN DESACTIVADAS.';

 document.getElementById('mensajeConfig').style.color=
  '#00ff91';
}

// ==========================================
// INICIALIZACION
// ==========================================

cargarHistorial();
actualizarBorrar();
actualizarVisibilidadCiclica();

</script>
</body>
</html>
)rawliteral";

// ==========================================
// SETUP
// ==========================================

void setup() {

 Serial.begin(115200);

 // Estado seguro de la etapa de potencia ANTES de habilitar la salida.
 // HIGH = totem pole DESACTIVADO.
 digitalWrite(PIN_TOTEM_POLE, HIGH);
 pinMode(PIN_TOTEM_POLE, OUTPUT);

 // Pulsadores físicos activos en LOW.
 for (uint8_t i = 0; i < sizeof(pinesPulsadores) / sizeof(pinesPulsadores[0]); i++) {
  pinMode(pinesPulsadores[i], INPUT_PULLUP);
 }

 // Inicializar buses con los GPIO asignados.
 Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
 SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
 pinMode(PIN_SD_CS, OUTPUT);
 digitalWrite(PIN_SD_CS, HIGH); // microSD deseleccionada hasta integrar el modulo

 Serial.println("MAPA GPIO INICIALIZADO");
 Serial.println("TOTEM POLE: HIGH = DESACTIVADO / LOW = ACTIVADO");
 Serial.println("I2C: SDA=21 SCL=22 | SPI microSD: CS=4 SCK=18 MISO=19 MOSI=23");

 WiFi.mode(WIFI_AP);
 WiFi.softAP(ssid,password);

 Serial.println();
 Serial.println("PROBADOR DC WEB 06");
 Serial.print("IP: ");
 Serial.println(WiFi.softAPIP());

 server.on("/",HTTP_GET,[](){

  server.send_P(200,"text/html",pagina);

 });

 // ========================================
 // CONFIGURACION PROTEGIDA
 // ========================================

 server.on("/config",HTTP_GET,[](){

  if(!server.authenticate(usuarioWeb,claveWeb)){
   server.requestAuthentication();
   return;
  }

  server.send(200,"text/plain","ACCESO AUTORIZADO");

 });

 server.begin();

 Serial.println("SERVIDOR WEB INICIADO");
}

// ==========================================
// LOOP
// ==========================================

void loop() {
 server.handleClient();
}
