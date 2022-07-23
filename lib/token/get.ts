import getCookie from '../cookie/get'

const getToken = (header: string) => getCookie(header, 'token')

export default getToken
