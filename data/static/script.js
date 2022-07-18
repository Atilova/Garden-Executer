const l = (...args) => console.log("Info -> ", ...args);


const configWorkflow = () => {
  const $configIndoorLi = document.querySelector("#config-indoor"),
        $configNoiseLi = document.querySelector("#config-noise"),
        $configDisturberLi = document.querySelector("#config-disturber"),
        $configThresholdLi = document.querySelector("#config-threshold"),
        $configSpikeLi = document.querySelector("#config-spike"),
        $configLightningsLi = document.querySelector("#config-lightnings");

  const $applyConfigButton = document.querySelector("#config-apply");
    
  const fetchConfig = () => {
    fetch("http://192.168.1.112/api/options",)
      .then(response => response.json())
      .then(data => {
        setSelectOption($configIndoorLi, data.indoor ? "indoor" : "outdoor");
        setSelectOption($configNoiseLi, data.noiseLevel);
        setSelectOption($configDisturberLi, data.showDisturber ? "on" : "off");
        setSelectOption($configThresholdLi, data.watchdogThreshold);
        setSelectOption($configSpikeLi, data.spike);
        setSelectOption($configLightningsLi, data.lightningsCount);
      });
  };

  const setLoader = (state, $parentElement) => {    
    $parentElement.querySelector(".spark-options__loader").style.display = state ? "block" : "none";
    $parentElement.querySelector("select").style.display = state ? "none" : "block";

    const loadingClass = "spark-options__setting_loading";
    setTimeout(() => state ? $parentElement.classList.add(loadingClass): $parentElement.classList.remove(loadingClass), 0);
  };

  const getSelectElementValue = $parentElement => {    
    return $parentElement.querySelector("select").value;
  };

  const setSelectOption = ($parentElement, value) => {    
    $parentElement.querySelector("select").value = value;
    setLoader(false, $parentElement);    
  };

  $applyConfigButton.addEventListener("click", () => {
    jsonToServer = JSON.stringify({
      "indoor": getSelectElementValue($configIndoorLi) == "indoor" ? true : false,
      "noiseLevel": +getSelectElementValue($configNoiseLi),
      "showDisturber": getSelectElementValue($configDisturberLi) == "on" ? true : false,
      "watchdogThreshold": +getSelectElementValue($configThresholdLi),
      "spike": +getSelectElementValue($configSpikeLi),
      "lightningsCount": +getSelectElementValue($configLightningsLi)
    });

    fetch("http://192.168.1.112/api/options", {"method": "POST", "body": jsonToServer});
  });

  fetchConfig();
};



configWorkflow();



// let ws = new WebSocket("ws://192.168.1.112/ws/");
// const $span = document.querySelector("#state");
// let timeoutId;


  // l("here");
  // const as = document.querySelector(".spark-options__setting");
  // as.classList.toggle("spark-options__setting_hiden");
// });

// function clearText()
//   {
//     $span.innerText = "ничего не обнаружено";
//   };

// function a(event)
//   {
//     l("OPEN -> ", event);
//   }

// function b(event)
//   {
//     l("CLOSE -> ", event);
//     // $span.innerText = ;
//   }

// function c(event)
//   {
//     l("ERROR -> ", event);
//   }

// function d(event)
//   {
//     // l($span);
//     clearTimeout(timeoutId);

//     // timeoutId = setTimeout(clearText, 3000);
//     // const value = JSON.parse(event.data).value
//     // $span.innerText = value;
//   }

// ws.onopen = a;
// ws.onclose = b;
// ws.onerror = c;
// ws.onmessage = d;


// fetchConfig();




// if($parentElement.classList.contains("spark-options__setting_toggle"))
// {
//   $parentElement.querySelector(".spark-options__loader").style.display = "none";
//   $parentElement.querySelector(".spark-options__loader select").style.display = "block";
// }
// else
//   {
//     $parentElement.querySelector(".spark-options__loader").style.display = "block";
//     $parentElement.querySelector(".spark-options__loader select").style.display = "none";
//   };



// setTimeout(
//   () => $element.classList.toggle("spark-options__setting_toggle"),
//   0
// );

