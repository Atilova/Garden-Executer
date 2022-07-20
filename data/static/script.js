const l = (...args) => console.log("Info -> ", ...args);


const managementWorkflow = () => {
  class FetchingState {
    static LOADING = 0;
    static FETCHED = 1;
    static FAILED = -1;
  };
  
  const configArray = [
    [document.querySelector("#config-indoor"), "indoor", ["outdoor", "indoor"]],
    [document.querySelector("#config-noise"), "noiseLevel"],
    [document.querySelector("#config-disturber"), "showDisturber", ["off", "on"]],
    [document.querySelector("#config-threshold"), "watchdogThreshold"],
    [document.querySelector("#config-spike"), "spike"],
    [document.querySelector("#config-lightnings"), "lightningsCount"],
  ];

  const $applyConfigButton = document.querySelector("#config-apply"),
        $refreshConfigButton = document.querySelectorAll("#config-refresh"),
        $editConfigButton = document.querySelector("#config-edit"),
        $exitConfigButton = document.querySelector("#config-exit");

  const fetchConfig = () => {
    fetch("http://192.168.1.112/api/options")
      .then(response => response.json())
      .then(data => {
        configArray.forEach(setting => {
          const [$htmlElement, name, params] = setting;
          setSelectOption($htmlElement, !!params ? params[data[name]] : data[name]);
        });
      })
      .catch(error => setTimeout(
        () => configArray.forEach(setting => setLoader(FetchingState.FAILED, setting[0])),
        100
      ));
  };

  const setLoader = (state, $parentElement) => {
    const loadingClass = "spark-options__setting_loading";

    const setDisplayAll = ($select, $loader, $error) => {      
      $parentElement.querySelector("select").style.display = $select ? "block" : "none";
      $parentElement.querySelector(".spark-options__loader").style.display = $loader ? "block" : "none";
      $parentElement.querySelector("i").style.display = $error ? "block" : "none";
    };

    if(state == FetchingState.LOADING) {
      setDisplayAll(false, true, false);
      setTimeout(() => {
        $parentElement.classList.remove("spark-options__setting_failed");
        $parentElement.classList.add(loadingClass);
      }, 50);
    }
    else if(state == FetchingState.FETCHED) {
      setDisplayAll(true, false, false);      
      setTimeout(() => {
        $parentElement.classList.remove("spark-options__setting_failed");
        $parentElement.classList.remove(loadingClass);
      });
    }
    else if(state == FetchingState.FAILED) {
      setDisplayAll(false, false, true);
      setTimeout(() => {
        $parentElement.classList.remove(loadingClass);
        $parentElement.classList.add("spark-options__setting_failed");
      }, 50);
    };
  };

  const getSelectElement = $parentElement => {
    return $parentElement.querySelector("select");
  };

  const setSelectOption = ($parentElement, value) => {
    $parentElement.querySelector("select").value = value;
    setLoader(FetchingState.FETCHED, $parentElement);
  };

  const allowEdit = allow => {
    configArray.forEach(setting => {
      const $element = getSelectElement(setting[0]);

      $element.disabled = !allow;
      if(allow)
        $element.style.textAlign = "center";
      else
        $element.style.textAlign = "end";
      });

    document.querySelectorAll(".spark-options__menu").forEach(($menu) => {
      allow ? $menu.classList.add("spark-options__menu_next") : $menu.classList.remove("spark-options__menu_next");
    });
  };

  const refreshConfig = () => {
    configArray.forEach(setting => setLoader(FetchingState.LOADING, setting[0]));
    setTimeout(fetchConfig, 150);
  };

  $applyConfigButton.addEventListener("click", () => {
    jsonToServer = {};
    configArray.forEach(setting => {
      const [$htmlElement, name, params] = setting,
            currentValue = getSelectElement($htmlElement).value;
      jsonToServer[name] = !!params ? !!params.indexOf(currentValue) : +currentValue;
      setLoader(FetchingState.LOADING, $htmlElement);
    });
    fetch("http://192.168.1.112/api/options", {"method": "POST", "body": JSON.stringify(jsonToServer)});
    setTimeout(fetchConfig, 150);
  });

  $refreshConfigButton.forEach($button => $button.addEventListener("click", refreshConfig));

  $editConfigButton.addEventListener("click", () => allowEdit(true));

  $exitConfigButton.addEventListener("click", () => {
    allowEdit(false);
    refreshConfig();
  });
  
  fetchConfig();
};

managementWorkflow();



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




// $parentElement.querySelector(".spark-options__loader").style.display = state == FetchingState.LOADING ? "block" : "none";
// $parentElement.querySelector("select").style.display = state == FetchingState.LOADING ? "none" : "block";

// const loadingClass = "spark-options__setting_loading";
// setTimeout(() => state == FetchingState.LOADING ? $parentElement.classList.add(loadingClass): $parentElement.classList.remove(loadingClass), 0);