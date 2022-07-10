import overlay from './store'

const hideOverlay = () => {
	overlay.set(null)
}

export default hideOverlay
