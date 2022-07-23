import { parse } from 'cookie'
import { unsign } from 'cookie-signature'

const getCookie = (header: string, name: string) => {
	const value = parse(header)[name]
	return (value && unsign(value, import.meta.env.VITE_COOKIE_SECRET)) || null
}

export default getCookie
