document.addEventListener('DOMContentLoaded', () => {
	const intersectionObserver = new IntersectionObserver((entries) => {
		for(const entry of entries){
			updateSliderDimensions(entry.target)
		}
	})

	const resizeObserver = new ResizeObserver((entries) => {
		for(const entry of entries){
			updateSliderDimensions(entry.target)
		}
	})


	const sliders = document.querySelectorAll(".comparison-slider")
	for (const slider of sliders) {
		updateSliderDimensions(slider)
		addSliderDragBehaviour(slider);
		intersectionObserver.observe(slider)
		resizeObserver.observe(slider)
	}

	function updateSliderDimensions(slider){
		const imgWidth = slider.querySelector(':scope > img').getBoundingClientRect().width + "px"
		const resize = slider.querySelector('.resize')
		resize.querySelector("img").style.width = imgWidth
	}

	/**
	* @param {HTMLElement} slider
	*/
	function addSliderDragBehaviour(slider) {
		const dragElement = slider.querySelector('.divider')
		const resizeElement = slider.querySelector('.resize')

		let dragWidth, containerOffset, containerWidth, minLeft, maxLeft

		const pointermovehandler = function(event) {

			// if the user is using mouse, use preventDefault to prevent the user from
			// selecting the images as he moves the silder arround.
			if ( event.pointerType === "mouse" ) {
				event.preventDefault();
			}
			
			let moveX = event.clientX
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

		dragElement.addEventListener("pointerdown", () => {
			
			dragElement.classList.add("dragging");
			resizeElement.classList.add("resizing");

			dragWidth = dragElement.getBoundingClientRect().width;
			containerOffset = slider.getBoundingClientRect().left;
			containerWidth = slider.getBoundingClientRect().width;
			minLeft = containerOffset + 10;
			maxLeft = containerOffset + containerWidth - dragWidth - 10;

			window.addEventListener("pointermove", pointermovehandler)

			window.addEventListener("pointerup", () => {
				// stop clicping the image and move the slider
				dragElement.classList.remove("dragging");
				resizeElement.classList.remove("resizing");
				window.removeEventListener("pointermove", pointermovehandler)
			}, { once: true });
		})
	}
});
