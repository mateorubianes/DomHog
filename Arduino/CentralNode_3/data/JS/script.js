/* - - - - USUARIO Y CONTRASENA - - - - */
/*
function popup()
{
    var usuarioHASH = 1101140680;
    var contrasenaHASH = 1963471916;
    
    let text;
    let usuario = prompt("Ingresar Usuario", "Usuario");

    //check_usuario(usuario);


let contrasena = prompt("Ingresar Contrasena", "Contrasena");

    if ((check_usuario(usuario) == usuarioHASH) && 
        (check_contrasena(contrasena) == contrasenaHASH)) 
        {
            //text = "Contrasena correcta";
        }
        else{
            text = "Contrasena incorrecta";
            alert(text);
            window.location.href = 'index.html';
        }
}

function check_usuario(string)
{
    var usuarioCHECK = 0;
    if (string.length == 0) return usuarioCHECK;
    for (i = 0; i < string.length; i++) 
    {
    char = string.charCodeAt(i);
    usuarioCHECK = ((usuarioCHECK << 5) - usuarioCHECK) + char;
    usuarioCHECK = usuarioCHECK & usuarioCHECK;
    }
    return usuarioCHECK;
}

function check_contrasena(string)
{
        var usuarioCHECK = 0;
        if (string.length == 0) return usuarioCHECK;
        for (i = 0; i < string.length; i++) 
        {
        char = string.charCodeAt(i);
        usuarioCHECK = ((usuarioCHECK << 5) - usuarioCHECK) + char;
        usuarioCHECK = usuarioCHECK & usuarioCHECK;
        }
        return usuarioCHECK;
}*/

function popup()
{
  document.getElementById("ENC").disabled = true;
}

/* - - - - PARAMETROS SENSADOS - - - - */
function tempsen(element)
{
  var output = document.getElementById("tempsen");
  output.innerHTML = element.value;
}

function humsen(element)
{
  var output = document.getElementById("humsen");
  output.innerHTML = element.value;
}

/* - - - - SLIDER TEMPERATURA - - - - */
function slider_Temp(element)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/sliderTemp?ST=" + element.value, true);
  document.getElementById("Temp").innerHTML = element.value;
  xhr.send();
}

/* - - - - SLIDER LUZ DIMERIZABLE - - - - */
function slider_LuzDim(element)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/sliderLuzDim?SL=" + element.value, true);
  document.getElementById("LuzDim").innerHTML = element.value;
  xhr.send();
}

/* - - - - SLIDER PERSIANA - - - - */
function slider_Per(element)
{
  var xhr = new XMLHttpRequest();
  switch(element.value)
  {
    case "0":
      xhr.open("GET", "/sliderPer?SP=0", true);
      document.getElementById("Per").innerHTML = "BAJA";
      break;
    case "1":
      xhr.open("GET", "/sliderPer?SP=1", true);
      document.getElementById("Per").innerHTML = "MEDIA";
      break;
    case "2":
      xhr.open("GET", "/sliderPer?SP=2", true);
      document.getElementById("Per").innerHTML = "ALTA";
      break;  
  }
  xhr.send();
}

/* - - - - BOTON ENCHUFE - - - - */
function EncEnc(element)
{
  var xhr = new XMLHttpRequest();
  if(element.checked)
  {
    xhr.open("GET", "/enchufe?E1=1", true); 
  }
  else 
  {
     xhr.open("GET", "/enchufe?E1=0", true); 
  }
  xhr.send();
}

/* - - - - BOTON LUZ E/A - - - - */
function EncLuzEA(element)
{
  var xhr = new XMLHttpRequest();
  if(element.checked)
  {
    xhr.open("GET", "/luzEA?L1=1", true); 
  }
  else 
  {
     xhr.open("GET", "/luzEA?L1=0", true); 
  }
  xhr.send();
}

control = 0;

function BotCon (element)
{

  switch (control) 
  {
    case 0:
      document.getElementById("control").innerHTML = "CONTROL MANUAL";
      document.getElementById("inicio").style.display = 'none';
      document.getElementById("fin").style.display = 'none';
      document.getElementById("labelFin").style.display = 'none';
      document.getElementById("labelInicio").style.display = 'none';
      document.getElementById("botonHora").style.display = 'none';
      document.getElementById("DIM").style.display = 'block';
      document.getElementById("labelDim").style.display = 'block';
      document.getElementById("EA").style.display = 'block';
      document.getElementById("labelEA").style.display = 'block';
      break;
    case 1:
      document.getElementById("control").innerHTML = "CONTROL POR HORA";
      document.getElementById("inicio").style.display = 'block';
      document.getElementById("fin").style.display = 'block';
      document.getElementById("labelFin").style.display = 'block';
      document.getElementById("labelInicio").style.display = 'block';
      document.getElementById("botonHora").style.display = 'block';
      document.getElementById("DIM").style.display = 'none';
      document.getElementById("labelDim").style.display = 'none';
      document.getElementById("EA").style.display = 'none';
      document.getElementById("labelEA").style.display = 'none';
      break;
  }
  control ++;
  if (control >= 2)
    control = 0;
}

function envio_ini()
{
  element = document.getElementById("inicio");
  var xhr = new XMLHttpRequest();
  document.getElementById("inicio").innerHTML = element.value;
  xhr.open("GET", "/hora?HI=" + element.value, true);
  console.log(element.value);
  xhr.send();
}

function envio_fin()
{
  element = document.getElementById("fin");
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/hora?HF=" + element.value, true);
  console.log(element.value);
  document.getElementById("fin").innerHTML = element.value;
  xhr.send();
}

function envio_total()
{
  envio_ini();
  envio_fin();
}

if (!!window.EventSource) 
{
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
   console.log("Events Connected");
  }, false);
  source.addEventListener('error', function(e) {
   if (e.target.readyState != EventSource.OPEN) {
     console.log("Events Disconnected");
   }
  }, false);
  
  source.addEventListener('message', function(e) {
   console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
   console.log("new_readings", e.data);
   /*var obj = JSON.parse(e.data);
   document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
   document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
   document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
   document.getElementById("rh"+obj.id).innerHTML = obj.readingId;*/
   //BotCon (element);
   //EncLuzEA(element);
   document.location.reload(true);
  }, false);
 }