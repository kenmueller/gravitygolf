import { encrypt, decrypt } from './hash'

export const get = (key: string) => {
	const value = localStorage.getItem(key)
	if (value === null) return null

	return decrypt(value)
}

export const set = (key: string, value: string) => {
	localStorage.setItem(key, encrypt(value))
}

export const remove = (key: string) => {
	localStorage.removeItem(key)
}
