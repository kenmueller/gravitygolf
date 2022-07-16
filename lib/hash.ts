import AES from 'crypto-js/aes.js'
import utf8 from 'crypto-js/enc-utf8.js'

export const encrypt = (value: string) =>
	AES.encrypt(value, import.meta.env.VITE_HASH).toString()

export const decrypt = (value: string) =>
	AES.decrypt(value, import.meta.env.VITE_HASH).toString(utf8)
