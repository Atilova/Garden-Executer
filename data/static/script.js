const l = (...args) => console.log("Info -> ", ...args);


const uuid4 = () => ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, c =>
  (c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> c / 4).toString(16)
);

const managementWorkflow = () => {
  const RECONNECT_TIMEOUT = 6,  // In seconds
        PING_INTERVAL = 10;  // In seconds

  let ws;

  let lastPingId,
      lastDetectedTimeoutId,
      isPingFailed = false,
      allowToPing = false;

  class FetchingState {
    static LOADING = 0;
    static FETCHED = 1;
    static FAILED = -1;
  };

  class InboxType {
    static PONG = "PONG";
    static WEATHER = "WEATHER";
    static STATUS  = "STATUS";
    static CONFIG_UPDATED = "CONFIG_UPDATED";
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
    configArray.forEach(setting => setLoader(FetchingState.LOADING, setting[0]));
    setTimeout(() => {
      fetch("http://192.168.1.106/api/options")
        .then(response => response.json())
        .then(data => {
          configArray.forEach(setting => {
            const [$htmlElement, name, params] = setting;
            setSelectOption($htmlElement, !!params ? params[data[name]] : data[name]);
          });
        })
        .catch(error => setTimeout(() => configArray.forEach(setting => setLoader(FetchingState.FAILED, setting[0])), 100));
    }, 200);
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

  const getSelectElement = $parentElement => $parentElement.querySelector("select");

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

  $applyConfigButton.addEventListener("click", () => {
    jsonToServer = {};
    configArray.forEach(setting => {
      const [$htmlElement, name, params] = setting,
            currentValue = getSelectElement($htmlElement).value;
      jsonToServer[name] = !!params ? !!params.indexOf(currentValue) : +currentValue;
    });
    fetch("http://192.168.1.106/api/options", {"method": "POST", "body": JSON.stringify(jsonToServer)});
    fetchConfig();
  });

  $refreshConfigButton.forEach($button => $button.addEventListener("click", fetchConfig));

  $editConfigButton.addEventListener("click", () => allowEdit(true));

  $exitConfigButton.addEventListener("click", () => {
    allowEdit(false);
    fetchConfig();
  });

  const setDetected = status => {
    const stateModifiers = [
      ["NULL", "spark-state__message_null", "null"],
      ["NOT_DETECTED", "spark-state__message_null", "Not detected"],
      ["UNTRACKED", "spark-state__message_untracked", "Untracked"],
      ["DISTURBER", "spark-state__message_disturber", "Disturber"],
      ["NOISE", "spark-state__message_noise", "Noise"]
    ];

    const $locator = document.querySelector("#state");
    stateModifiers.forEach(modifier => $locator.classList.remove(modifier[1]));

    const currentModifier = stateModifiers.filter(modifier => modifier[0] == status)[0];
    $locator.classList.add(currentModifier[1]);
    $locator.innerHTML = currentModifier[2];
  };

  const setLightningUpdate = (distance, energy, time) => {
    if(!!!time)
      time = new Date().toLocaleTimeString(navigator.language, {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit"
      })

    document.querySelector("#lightning-time").innerText = time;
    document.querySelector("#lightning-distance").innerText = distance;
    document.querySelector("#lightning-energy").innerText = energy;
  };

  let lastLightningAnimationTimeout;
  const animateLightning = () => {
    const $lightningAnimation = document.querySelector(".lightning-animation");
    const startAnimation = () => {
      $lightningAnimation.classList.add('lightning-animation_strike');
      document.querySelector('body').style.background = "linear-gradient(-233deg, #474747ed 0%, #2d2d2d 80%)";
    };

    const stopAnimation = () => {
      $lightningAnimation.classList.remove('lightning-animation_strike');
      document.querySelector('body').style.background = "";    
    };

    clearTimeout(lastLightningAnimationTimeout);
    startAnimation();
    lastLightningAnimationTimeout = setTimeout(() => {
      stopAnimation();
      lastLightningAnimationTimeout = setTimeout(() => {
        startAnimation();
        lastLightningAnimationTimeout = setTimeout(stopAnimation, 170);
      }, 20);
    }, 170);
  };

  const setWeather = (temperature, humidity, pressure) => {
    document.querySelector(".weather-list__parameter_temperature .weather__option-value span").innerText = temperature;
    document.querySelector(".weather-list__parameter_temperature_humidity .weather__option-value span").innerText = humidity;
    document.querySelector(".weather-list__parameter_temperature_pressure .weather__option-value span").innerText = pressure;
  };

  const pong = confirmationId => {
    if(confirmationId == lastPingId)
      isPingFailed = false;
  };

  const ping = () => {
    lastPingId = uuid4();
    if(!allowToPing || ws.readyState != ws.OPEN)
      return;

    if(isPingFailed)
      return ws.close();

    isPingFailed = true;
    setTimeout(ping, PING_INTERVAL*1000);
    setTimeout(() => ws.readyState == ws.OPEN ?  ws.send(JSON.stringify({"type": "PING", "payload": lastPingId})) : null, 0);
  };

  const onOpen = event => {
    setSocketState(false);
    setDetected("NOT_DETECTED");
    fetchConfig();

    allowToPing = true;
    ping();
  };

  const onClose = event => {
    setSocketState(true);
    setWeather("null", "null", "null");
    setLightningUpdate("null", "null", "null");
    setDetected("NULL");

    allowToPing = false;
    setTimeout(startNewWs, RECONNECT_TIMEOUT*1000);
  };

  const onInbox = event => {
    const data = event.data,
          {type, payload} = JSON.parse(data);

    switch(type) {
      case InboxType.PONG: {
        pong(payload);
        break;
      };

      case InboxType.STATUS: {
        if(payload.status == "LIGHTNING") {
          setLightningUpdate(payload.data.distance, payload.data.energy);
          return animateLightning();
        };

        clearTimeout(lastDetectedTimeoutId);
        setDetected(payload.status);
        lastDetectedTimeoutId = setTimeout(() => setDetected("NOT_DETECTED"), 1000);
        break;
      };

      case InboxType.WEATHER: {
        setWeather(payload.temperature, payload.humidity, payload.pressure);
        break;
      };

      case InboxType.CONFIG_UPDATED: {
        fetchConfig();
        break;
      };
    };
  };

  const startNewWs = () => {
    ws = new WebSocket("ws://192.168.1.106/ws/");
    ws.onopen = onOpen;
    ws.onclose = onClose;
    ws.onmessage = onInbox;
    isPingFailed = false;
  };

  const setSocketState = isConnecting => {
    const $display =  document.querySelector(".page__header-socket");
    setTimeout(() => {
      if(isConnecting)
        return $display.classList.remove("page-header__socket_online");
      $display.classList.add("page-header__socket_online");
    }, 200);
  };


  // let $play = document.createElement(`button`);
  // $play.innerText = "PLay";
  // $play.addEventListener("click", animateLightning);
  // document.querySelector("body").insertAdjacentElement(
  //   "beforeend",
  //   $play
  // );

  startNewWs();
};

// managementWorkflow();




test.addEventListener("click", () => {
  // const $value = document.getElementById("lightning-distance");
  // const $value = document.getElementById("lightning-locator");
  // $value.setAttribute("disabled", false);

  const $value = document.querySelector("#config-noise");
  // $value.disabled = !$value.disabled;
  $value.classList.toggle("spark-dashboard__holder-container_loaded");
  
  
});
