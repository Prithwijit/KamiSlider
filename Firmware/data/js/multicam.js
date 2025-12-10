// Initialize an empty array to store the objects
let multiPoints = [];
function updateMultiObj(multiPoint,i ) {
    // Send 
    $.ajax({
        url: "/api/updateMulti",
        type: "get", //send it through get method
        data: {
            x: multiPoint.x,
            y: multiPoint.y,
            z: multiPoint.z,
            i: i,
        },
        success: function (response) {
            console.log(response);
        },
        error: function (xhr) {
            //Do Something to handle error
            console.log(xhr);
        }
    });
}
function addMultiObj(multiPoint ) {
    // Send 
    $.ajax({
        url: "/api/addMulti",
        type: "get", //send it through get method
        data: {
            x: multiPoint.x,
            y: multiPoint.y,
            z: multiPoint.z
        },
        success: function (response) {
            console.log(response);
        },
        error: function (xhr) {
            //Do Something to handle error
            console.log(xhr);
        }
    });
}

function deleteMultiObj(i ) {
    // Send 
    $.ajax({
        url: "/api/deleteMulti",
        type: "get", //send it through get method
        data: {
            i:i
        },
        success: function (response) {
            console.log(response);
        },
        error: function (xhr) {
            //Do Something to handle error
            console.log(xhr);
        }
    });
}

function multiMoveStart() {
    // Send 
    $.ajax({
        url: "/api/startMulti",
        type: "get", //send it through get method
        success: function (response) {
            console.log(response);
        },
        error: function (xhr) {
            //Do Something to handle error
            console.log(xhr);
        }
    });
}
// Function to calculate the total duration in seconds
function calculateDurationInSeconds(block) {
    let hours = parseInt($("#inputHoursMulti").val()) || 0;
    let minutes = parseInt($("#inputMinutesMulti").val()) || 0;
    let seconds = parseInt($("#inputSecondsMulti").val()) || 0;
    return seconds + minutes * 60 + hours * 3600;
}

// Function to add a new object and its corresponding form block
function addObject() {
    let pos = parseFloat($("#sliderManualPositionMulti").val()) || 0;
    let rot = parseFloat($("#sliderManualRotationMulti").val()) || 0;
    let duration = calculateDurationInSeconds(); // calculate the initial duration

    // Add the new object to the array
    const newObject = { x: pos, y: rot, z: duration };
    multiPoints.push(newObject);
    
    addMultiObj(newObject);
    // Render the updated multiPoints array
    renderMultiPoints();
    updateTotalTime();
}
function updateTotalTime() {
    var totalSecs = 0;
    for (i = 0; i < multiPoints.length; i++) {
        totalSecs+= multiPoints[i].z;
    }
    const { hours, minutes, remainingSeconds } = convertSecondsToHMS(totalSecs);
    // Update the input fields for hours, minutes, and seconds
    document.getElementById('inputHoursMultiTotal').value = hours;
    document.getElementById('inputMinutesMultiTotal').value = minutes;
    document.getElementById('inputSecondsMultiTotal').value = remainingSeconds;
}
// Function to update an object
function updateObject(i) {
    if (i >= 0 && i < multiPoints.length) {
        let pos = parseFloat($("#sliderManualPositionMulti").val()) || 0;
        let rot = parseFloat($("#sliderManualRotationMulti").val()) || 0;
        let duration = calculateDurationInSeconds(); // calculate the updated duration

        // Update the object in the array
        multiPoints[i] = { x: pos, y: rot, z: duration };

        updateMultiObj(multiPoints[i],i);
        // Render the updated multiPoints array
        renderMultiPoints();
        // Update the value of #sliderManualPositionMulti and its label
        const slider = document.getElementById('sliderManualPositionMulti');
        slider.value = x;

        const label = document.querySelector('label[for="labelManualPositionMulti"]');
        label.innerHTML = `Position: ${x} mm`;


        // Update the value of #sliderManualRotationMulti and its label
        const sliderRotation = document.getElementById('sliderManualRotationMulti');
        sliderRotation.value = y;
        const labelRotation = document.querySelector('label[for="sliderManualRotationMulti"]');
        labelRotation.innerHTML = `Rotate camera for: ${y} °`;

        // Convert total seconds to hours, minutes, and seconds
        const { hours, minutes, remainingSeconds } = convertSecondsToHMS(z);

        // Update the input fields for hours, minutes, and seconds
        document.getElementById('inputHoursMulti').value = hours;
        document.getElementById('inputMinutesMulti').value = minutes;
        document.getElementById('inputSecondsMulti').value = remainingSeconds;
        //const addButton = document.getElementById('btnMultiSlideAdd');
        //addButton.disabled = false;
        
        updateTotalTime();
    }
}

// Function to remove the i-th object
function removeObject(i) {
    if (i >= 0 && i < multiPoints.length) {
        multiPoints.splice(i, 1); // Remove the i-th element from the array
        deleteMultiObj(i);
        // Render the updated multiPoints array
        renderMultiPoints();
    }
    if (multiPoints.length == 0) {
        // Get the element by ID
        const element = document.getElementById('multiDur');

        // Set the display style to flex
        element.style.display = 'none';
    }
    
    updateTotalTime();
}

function readMultiStatus() {
   /* var i=0;
    var hasMore=true;
    while(hasMore){
        // Send */
        $.ajax({
            url: "/api/multi-slider-status",
            type: "get", //send it through get method
          /*  data: {
                i:i
            },*/
            success: function (response) {
                console.log(response);

                var jsonResponse = jQuery.parseJSON(response);
                if(jsonResponse){
                    multiPoints=jsonResponse;
                }
                renderMultiPoints();
                updateTotalTime();
                console.log(jsonResponse);

            },
            error: function (xhr) {
               /* // Handle the error
                if (xhr.status === 404) {
                    hasMore=false;
                } else {
                    console.log("Error " + xhr.status + ": " + xhr.statusText);
                }*/
            }
        });
        /*i++;
    }*/
	
}


// Function to render the multiPoints array as cards
function renderMultiPoints() {
    const multiDisp = document.getElementById('multiDisp');
    multiDisp.innerHTML = ''; // Clear any existing content

    multiPoints.forEach((point, index) => {
        const card = document.createElement('div');
        card.className = 'card';
        if(index==0){
            card.innerHTML = `
            <h5>Point ${index + 1}</h5>
            <p>Position: ${point.x} mm</p>
            <p>Rotation: ${point.y} °</p>
            <p><br></p>
            <button onclick="removeObject(${index})" class="btn btn-danger">Remove</button>
            <button onclick="updateObject(${index})" class="btn btn-primary update-btn" disabled>Update</button>
          `;
        }else{
            card.innerHTML = `
            <h5>Point ${index + 1}</h5>
            <p>Position: ${point.x} mm</p>
            <p>Rotation: ${point.y} °</p>
            <div style="    transform: translateX(-100px);
    background: linear-gradient(90deg, #349aed, transparent);
    align-items: center;
    align-self: center;
}"><p>Duration: ${getHMS(point.z)}</p></div>
            <button onclick="removeObject(${index})" class="btn btn-danger">Remove</button>
            <button onclick="updateObject(${index})" class="btn btn-primary update-btn" disabled>Update</button>
          `;
        }
        

        card.addEventListener('click', () => {
            // Disable all "Update" buttons within the cards
            document.querySelectorAll('.update-btn').forEach(button => button.disabled = true);
            const updateButton = card.querySelector('.update-btn');
            updateButton.disabled = false;
            const addButton = document.getElementById('btnMultiSlideAdd');
            //addButton.disabled=true;
            // Set the updated position to the slider
            /*
                        // Read the content of this specific card
                        const pointNumber = card.querySelector('h5').textContent;
                        const position = card.querySelector('p:nth-child(2)').textContent;
                        const rotation = card.querySelector('p:nth-child(3)').textContent;
                        const duration = card.querySelector('p:nth-child(4)').textContent;
            
                        console.log('Card Content:');
                        console.log('Point:', pointNumber);
                        console.log('Position:', position);
                        console.log('Rotation:', rotation);
                        console.log('Duration:', duration);*/
            // Update the value of #sliderManualPositionMulti and its label
            const slider = document.getElementById('sliderManualPositionMulti');
            slider.value = point.x;

            const label = document.querySelector('label[for="labelManualPositionMulti"]');
            label.innerHTML = `Position: ${point.x} mm`;


            // Update the value of #sliderManualRotationMulti and its label
            const sliderRotation = document.getElementById('sliderManualRotationMulti');
            sliderRotation.value = point.y;
            const labelRotation = document.querySelector('label[for="sliderManualRotationMulti"]');
            labelRotation.innerHTML = `Rotate camera for: ${point.y} °`;

            // Convert total seconds to hours, minutes, and seconds
            const { hours, minutes, remainingSeconds } = convertSecondsToHMS(point.z);
            if (index > 0) {
                 // Get the element by ID
                 const element = document.getElementById('multiDur');

                 // Set the display style to flex
                 element.style.display = 'contents';
                // Update the input fields for hours, minutes, and seconds
                document.getElementById('inputHoursMulti').value = hours;
                document.getElementById('inputMinutesMulti').value = minutes;
                document.getElementById('inputSecondsMulti').value = remainingSeconds;
            } else {
                // Get the element by ID
                const element = document.getElementById('multiDur');

                // Set the display style to flex
                element.style.display = 'none';
            }

        });
        multiDisp.appendChild(card);
    });
    if (multiPoints.length > 0) {
        // Get the element by ID
        const element = document.getElementById('multiDur');

        // Set the display style to flex
        element.style.display = 'contents';
    }

}
function getHMS(seconds){
    var str='';
    const { hours, minutes, remainingSeconds } = convertSecondsToHMS(seconds);
    if(remainingSeconds){
        str=remainingSeconds+'s'+str;
    }
    if(minutes){
        str=minutes+'m '+str;
    }
    if(hours){
        str=hours+'h '+str;
    }
    return str;
}
// Function to convert total seconds into hours, minutes, and seconds
function convertSecondsToHMS(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const remainingSeconds = seconds % 60;
    return { hours, minutes, remainingSeconds };
}


// Add event listeners for Add, Update, and Remove buttons
document.getElementById('btnMultiSlideAdd').addEventListener('click', addObject);


// Add event listeners for Add, Update, and Remove buttons
document.getElementById('btnAutoSlideMulti').addEventListener('click', multiMoveStart);


// Add event listeners for Add, Update, and Remove buttons
document.getElementById('refreshMultiStatus').addEventListener('click', readMultiStatus);


document.getElementById('btnMultiSlideMoveMulti').addEventListener('click', () => {
    var posSlide = $("#sliderManualPositionMulti").val();
    var cfgSliderSpeed = $("#cfgSliderSpeed").val();
    var cfgSliderAccel = $("#cfgSliderAccel").val();

    var posRotate = $("#sliderManualRotationMulti").val();
    var cfgRotationSpeed = $("#cfgRotationSpeed").val();
    var cfgRotationAccel = $("#cfgRotationAccel").val();

    console.log("posSlide: " + posSlide);
    console.log("cfgSliderSpeed: " + cfgSliderSpeed);
    console.log("cfgSliderAccel: " + cfgSliderAccel);

    console.log("posRotate: " + posRotate);
    console.log("cfgRotationSpeed: " + cfgRotationSpeed);
    console.log("cfgRotationAccel: " + cfgRotationAccel);

    moveToPosition(posSlide, cfgSliderSpeed, cfgSliderAccel, posRotate, cfgRotationSpeed, cfgRotationAccel);
});
/*
document.getElementById('btnMultiSlideUpdate').addEventListener('click', () => {
    const indexToUpdate = 0; // Replace with the actual index you want to update
    updateObject(indexToUpdate);
});
document.getElementById('btnMultiSlideRemove').addEventListener('click', () => {
    const indexToRemove = 0; // Replace with the actual index you want to remove
    removeObject(indexToRemove);
});*/
