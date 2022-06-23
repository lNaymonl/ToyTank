let wrapper = window.getComputedStyle(document.querySelector("#wrapper"));

console.log(wrapper.marginTop);

// Reload window if resized
window.addEventListener(
  "resize",
  () => {
    location.reload();
  },
  true
);

// Get HTML-Elements
let shootBtn = document.getElementById("shootBtn");
let btnState = false;
let btn_time = 500;
let btn_current_time;
let btn_last_time = 0;

let aimField = document.getElementById("aim");

let joystickOuter = document.getElementById("joystickOuter");
let joystickDiv = document.getElementById("joystickDiv");
let joystickInner = document.getElementById("joystickInner");
let joystickInnerContent = document.getElementById("joystickInnerContent");

let joystickSize = 12.5;

let aim_old = aimField.value;

let touchedObj = new Array();
let touchIndex = (touchObj) =>
  touchedObj.map((obj) => obj.target).indexOf(touchObj.target);

// Set Joystick properties
joystickDiv.style.width = joystickSize * 2 + "vw";
joystickOuter.style.width = joystickSize * 2 + "vw";
joystickInner.style.left = joystickSize / 2 + "vw";
joystickInner.style.top = joystickSize / 2 + "vw";
joystickInnerContent.style.width = joystickSize + "vw";

// Create new Obj of class JoystickController (defined in joy.js)
let joystick = new JoystickController(
  "joystickInner",
  joystickInnerContent.clientWidth,
  8
);

let joystick_value_old = joystick.value;

// Function to send Data to Server
function sendRequest(urlSide = "", param = "") {
  var xhr = new XMLHttpRequest();

  var url = new URL("http://192.168.1.1/" + urlSide + "?" + param);

  xhr.open("GET", url, true);

  xhr.send();
}

// Function to convert float to two decimal places
function twoDP(num) {
  return Math.round((num + Number.EPSILON) * 100) / 100;
}

// Function to detect if mouse is down and moved on Obj
function mouseMoveWhilstDown(target, whileMove) {
  var endMove = function () {
    target.removeEventListener("mousemove", whileMove);
    target.removeEventListener("mouseup", endMove);
  };

  target.addEventListener("mousedown", function (event) {
    event.stopPropagation(); // remove if you do want it to propagate ..
    target.addEventListener("mousemove", whileMove);
    target.addEventListener("mouseup", endMove);
  });
}

// When mouse over aimField Obj (HTML-Obj) and down, get mouse-position
mouseMoveWhilstDown(aimField, function (event) {
  // console.log(event);
  aimField.value = {
    x: twoDP((100 / aimField.offsetWidth) * event.offsetX),
    y: twoDP((100 / aimField.offsetHeight) * event.offsetY),
  };
});

// Function to detect how many touches are activ and on which object the are
function getTouches(event) {
  event.preventDefault();

  touchedObj.length = 0;

  let touches = event.touches;
  for (let i = 0; i < touches.length; i++) {
    touchedObj.push(touches[i]);
  }
}

// "touchedObj" is getting updated
document.addEventListener("touchstart", (e) => getTouches(e));
document.addEventListener("touchend", (e) => getTouches(e));

// Function to get the position of an touch event, on an certain obj
function getTouchPos(touchObj) {
  let rect = touchObj.target.getBoundingClientRect();

  let touchIndexInFunc = touchIndex(touchObj);

  if (
    touchObj.touches[touchIndexInFunc].clientX - rect.left >= 0 &&
    touchObj.touches[touchIndexInFunc].clientY - rect.top >= 0 &&
    touchObj.touches[touchIndexInFunc].clientX - rect.left <=
      touchObj.target.offsetWidth &&
    touchObj.touches[touchIndexInFunc].clientY - rect.top <=
      touchObj.target.offsetHeight
  ) {
    touchObj.target.value = {
      x:
        (100 / touchObj.target.offsetWidth) *
        (touchObj.touches[touchIndexInFunc].clientX - rect.left),
      y:
        (100 / touchObj.target.offsetHeight) *
        (touchObj.touches[touchIndexInFunc].clientY - rect.top),
    };
  }
  // console.log(touchObj.target.value);
}

// Get touch-position on aimField Obj (HTML-Obj)
aimField.addEventListener("touchmove", (e) => {
  getTouchPos(e);
  // console.log(aimField.value);
});

// Every 500ms check if values of joystick and aimField changed
// if true: send it to Server
setInterval(() => {
  if (aimField.value != undefined && aim_old != aimField.value) {
    sendRequest(
      "aimField",
      "aimX=" + aimField.value.x + "&aimY=" + aimField.value.y
    );
    console.log("aimField: ", aimField.value);
    aim_old = aimField.value;
  }
  if (joystick_value_old != joystick.value) {
    joystick.value = {
      x: twoDP(joystick.value.x),
      y: twoDP(joystick.value.y),
    };
    console.log("joy: ", joystick.value);

    joystick_value_old = joystick.value;
    sendRequest(
      "joy",
      "joyX=" + joystick.value.x + "&joyY=" + joystick.value.y
    );
  }
}, 500);

// Check if mousedown on shootBtn-Obj (HTML-Obj)
// if true: send it to Server
shootBtn.addEventListener("mousedown", (e) => {
  btn_current_time = new Date().getTime();
  if (btn_current_time - btn_last_time > btn_time) {
    if (btnState == false) {
      console.log("BtnPressed: ", true);
      sendRequest("shootBtn", "btn=true");
      btnState = true;
    }
  }
});

// Check if mouseup on shootBtn-Obj (HTML-Obj)
// if true: send it to Server after 500ms
shootBtn.addEventListener("mouseup", (e) => {
  if (btn_current_time - btn_last_time > btn_time) {
    if (btnState == true) {
      setTimeout(() => {
        console.log("BtnPressed: ", false);
        sendRequest("shootBtn", "btn=false");
        btnState = false;
        btn_last_time = new Date().getTime();
      }, 50);
    }
  }
});

shootBtn.addEventListener("touchstart", (e) => {
  btn_current_time = new Date().getTime();
  if (btn_current_time - btn_last_time > btn_time) {
    if (btnState == false) {
      console.log("BtnPressed: ", true);
      sendRequest("shootBtn", "btn=true");
      btnState = true;
    }
  }
});

shootBtn.addEventListener("touchend", (e) => {
  if (btn_current_time - btn_last_time > btn_time) {
    if (btnState == true) {
      setTimeout(() => {
        console.log("BtnPressed: ", false);
        sendRequest("shootBtn", "btn=false");
        btnState = false;
        btn_last_time = new Date().getTime();
      }, 50);
    }
  }
});
