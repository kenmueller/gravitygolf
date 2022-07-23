const EMAIL_NAME_MATCH = /^(.+?)@/

const nameFromEmail = (email: string) =>
	email.match(EMAIL_NAME_MATCH)?.[1] ?? null

export default nameFromEmail
