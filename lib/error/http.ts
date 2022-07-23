import type ErrorCode from './code'

export default class HttpError extends Error {
	constructor(public code: ErrorCode, message: string) {
		super(message)
	}
}
