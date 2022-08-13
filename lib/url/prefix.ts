const prefixUrl = (url: string) =>
	MOBILE ? new URL(url, import.meta.env.VITE_PRODUCTION_ORIGIN).href : url

export default prefixUrl
