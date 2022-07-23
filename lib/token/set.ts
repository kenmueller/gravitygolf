import setCookie from '../cookie/set'

const setToken = (token: string | null) => setCookie('token', token)

export default setToken
