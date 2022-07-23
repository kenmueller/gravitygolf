import HttpError from '../http'
import DEFAULT_ERROR from '../default'

const errorFromResponse = async (response: Response) => {
	try {
		return new HttpError(response.status, await response.text())
	} catch {
		return new HttpError(response.status, DEFAULT_ERROR)
	}
}

export default errorFromResponse
