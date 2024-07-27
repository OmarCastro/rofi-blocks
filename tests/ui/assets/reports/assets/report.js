document.addEventListener('DOMContentLoaded', () => {
	const sliders = document.querySelectorAll(".comparison-slider")
	for (const slider of sliders) {
		const imgWidth = slider.querySelector(':scope > img').getBoundingClientRect().width + "px";
		const divider = slider.querySelector('.divider')
		const resize = slider.querySelector('.resize')
		resize.querySelector("img").style.width = imgWidth
		drags(divider, resize, slider);
		

		window.addEventListener("resize", function() {
			const imgWidth = slider.querySelector(':scope > img').getBoundingClientRect().width + "px";
			slider.querySelector(".resize img").style.width = imgWidth
		});
	}
});


/**
* @param {HTMLElement} dragElement
* @param {HTMLElement} resizeElement
* @param {HTMLElement} container
*/
function drags(dragElement, resizeElement, container) {

// This creates a variable that detects if the user is using touch input insted of the mouse.
let touched = false;
window.addEventListener('touchstart', function() { touched = true; });
window.addEventListener('touchend', function() { touched = false; });


let dragWidth, containerOffset, containerWidth, minLeft, maxLeft

const pointermovehandler = function(e) {
			
			// if the user is not using touch input let do preventDefault to prevent the user from slecting the images as he moves the silder arround.
			if ( touched === false ) {
				e.preventDefault();
			}
			
			let moveX = e.clientX
			let leftValue = moveX - dragWidth;

			// stop the divider from going over the limits of the container
			if (leftValue < minLeft) {
				leftValue = minLeft;
			} else if (leftValue > maxLeft) {
				leftValue = maxLeft;
			}

			let widthValue = (leftValue + dragWidth / 2 - containerOffset) * 100 / containerWidth + "%";

			document.querySelector(".dragging").style.left = widthValue
			document.querySelector(".resizing").style.width = widthValue;
			
		}

// clicp the image and move the slider on interaction with the mouse or the touch input
dragElement.addEventListener("pointerdown", function(e) {
		
		//add classes to the emelents - good for css animations if you need it to
		dragElement.classList.add("dragging");
		resizeElement.classList.add("resizing");
		//create vars
		dragWidth = dragElement.getBoundingClientRect().width;
		containerOffset = container.getBoundingClientRect().left;
		containerWidth = container.getBoundingClientRect().width;
		minLeft = containerOffset + 10;
		maxLeft = containerOffset + containerWidth - dragWidth - 10;
		
		//add event listner on the divider emelent
		dragElement.parentElement.addEventListener("pointermove", pointermovehandler)
	
	})
	
	
	dragElement.addEventListener("pointerup", function(e) {
		// stop clicping the image and move the slider
		dragElement.classList.remove("dragging");
		resizeElement.classList.remove("resizing");
		dragElement.parentElement.removeEventListener("pointermove", pointermovehandler)

	
	});
	
}