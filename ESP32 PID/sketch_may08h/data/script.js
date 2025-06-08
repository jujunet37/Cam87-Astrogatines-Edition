// Constants
const MAX_DATA_POINTS = 1000; // Keep all points
const DAY_MODE_COLORS = {
  text: '#333333',
  grid: 'rgba(0, 0, 0, 0.1)',
  series: '#00A6A6',
  tooltipText: '#333333',
  tooltipBg: '#ffffff',
  border: '#333333'
};
const NIGHT_MODE_COLORS = {
  text: '#ff0000',
  grid: 'rgba(255, 0, 0, 0.2)',
  series: '#ff0000',
  tooltipText: '#ff0000',
  tooltipBg: '#330000',
  border: '#ff0000'
};

// Track current zoom state
let currentZoomRange = null;
let isUserZoomed = false;

// Initialize chart
const chartT = new Highcharts.Chart({
  chart: { 
    renderTo: 'chart-temperature', 
    backgroundColor: 'transparent',
    zoomType: 'x',
    panning: true,
    panKey: 'shift',
    spacing: [20, 20, 20, 20],
    events: {
      selection: function(e) {
        isUserZoomed = !!e.xAxis;
      },
      click: function() {
        if (isUserZoomed && !this.resetZoomButton) {
          isUserZoomed = false;
          applyZoom(currentZoomRange);
        }
      }
    }
  },
  title: { text: '' },
  xAxis: {
    type: 'datetime',
    overscroll: 0,
    minRange: 5 * 60 * 1000,
    dateTimeLabelFormats: { 
      second: '%H:%M:%S',
      minute: '%H:%M',
      hour: '%H:%M',
      day: '%e. %b'
    },
    labels: { 
      style: { color: DAY_MODE_COLORS.text },
      align: 'center'
    },
    gridLineColor: DAY_MODE_COLORS.grid,
    events: {
      afterSetExtremes: function(e) {
        if (!e.trigger || e.trigger === 'zoom') {
          isUserZoomed = false;
        }
      }
    }
  },
  yAxis: {
    title: { 
      text: 'Temperature (°C)', 
      style: { color: DAY_MODE_COLORS.text },
      margin: 10
    },
    labels: { 
      style: { color: DAY_MODE_COLORS.text },
      align: 'center'
    },
    gridLineColor: DAY_MODE_COLORS.grid
  },
  tooltip: {
    style: { color: DAY_MODE_COLORS.tooltipText, fontSize: '12px' },
    backgroundColor: DAY_MODE_COLORS.tooltipBg,
    borderColor: DAY_MODE_COLORS.border,
    borderRadius: 5,
    borderWidth: 1,
    shared: true,
    crosshairs: true
  },
  plotOptions: {
    series: {
      marker: {
        enabled: true,
        radius: 3
      },
      states: {
        hover: {
          enabled: true,
          lineWidth: 2
        }
      }
    }
  },
  series: [{
    name: 'CCD Probe',
    type: 'line',
    color: DAY_MODE_COLORS.series,
    marker: { 
      symbol: 'circle', 
      radius: 3, 
      fillColor: DAY_MODE_COLORS.series 
    }
  }],
  credits: { enabled: false },
  legend: {
    align: 'center',
    verticalAlign: 'top',
    layout: 'horizontal'
  }
});

// Zoom function
function applyZoom(minutes) {
  const now = Date.now();
  if (minutes === 0) {
    currentZoomRange = null;
    isUserZoomed = false;
    if (chartT.series[0].data.length > 0) {
      const firstPoint = chartT.series[0].data[0].x;
      chartT.xAxis[0].setExtremes(firstPoint, now, true);
    }
  } else {
    currentZoomRange = minutes;
    isUserZoomed = false;
    chartT.xAxis[0].setExtremes(now - (minutes * 60 * 1000), now, true);
  }
}

// Setup zoom buttons
document.querySelectorAll('.zoom-button').forEach(button => {
  button.addEventListener('click', function() {
    const minutes = parseInt(this.getAttribute('data-minutes'));
    applyZoom(minutes);
  });
});

// Update chart colors
function updateChartColors(isNightMode) {
  const colors = isNightMode ? NIGHT_MODE_COLORS : DAY_MODE_COLORS;
  
  chartT.update({
    yAxis: {
      title: { style: { color: colors.text } },
      labels: { style: { color: colors.text } },
      gridLineColor: colors.grid
    },
    xAxis: {
      labels: { style: { color: colors.text } },
      gridLineColor: colors.grid
    },
    tooltip: {
      style: { color: colors.tooltipText },
      backgroundColor: colors.tooltipBg,
      borderColor: colors.border
    }
  });
  
  chartT.series[0].update({
    color: colors.series,
    marker: { fillColor: colors.series }
  });
}

// Plot temperature data
function plotTemperature(jsonValue) {
  try {
    const keys = Object.keys(jsonValue);
    const now = Date.now();
    
    keys.forEach((key, i) => {
      const y = Number(jsonValue[key]);
      if (isNaN(y)) return;
      
      chartT.series[i].addPoint([now, y], false, false);
      
      if (!isUserZoomed) {
        if (currentZoomRange !== null) {
          chartT.xAxis[0].setExtremes(
            now - (currentZoomRange * 60 * 1000), 
            now,
            false
          );
        } else {
          const extremes = chartT.xAxis[0].getExtremes();
          chartT.xAxis[0].setExtremes(
            extremes.min, 
            now,
            false
          );
        }
      }
    });
    
    chartT.redraw();
  } catch (error) {
    console.error('Error plotting temperature:', error);
  }
}

// Get current readings
async function getReadings() {
  try {
    const response = await fetch('/readings');
    if (!response.ok) throw new Error('Network response was not ok');
    const data = await response.json();
    plotTemperature(data);
  } catch (error) {
    console.error('Error fetching readings:', error);
  }
}

// Initialize EventSource
function initEventSource() {
  if (!window.EventSource) {
    console.log("EventSource not supported - falling back to polling");
    setInterval(getReadings, 2000);
    return;
  }

  const source = new EventSource('/events');
  
  source.addEventListener('open', () => console.log("Events Connected"));
  source.addEventListener('error', (e) => {
    if (e.target.readyState !== EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  });
  
  source.addEventListener('new_readings', (e) => {
    try {
      plotTemperature(JSON.parse(e.data));
    } catch (error) {
      console.error('Error processing new readings:', error);
    }
  });
}

// Initialize when page loads
window.addEventListener('load', () => {
  getReadings();
  initEventSource();
});