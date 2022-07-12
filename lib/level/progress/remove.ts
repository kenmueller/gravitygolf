const deleteLevelProgress = () => {
	try {
		localStorage.removeItem('stars')
		localStorage.removeItem('communityStars')
	} catch (error) {
		console.error(error)
	}
}

export default deleteLevelProgress
