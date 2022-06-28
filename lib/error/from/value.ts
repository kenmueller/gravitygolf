import HttpError from '..'
import ErrorCode from '../code'
import DEFAULT_ERROR from '../default'

const errorFromValue = <Value>(value: Value) =>
	value instanceof HttpError
		? value
		: new HttpError(
				ErrorCode.Internal,
				value instanceof Error ? value.message : DEFAULT_ERROR
		  )

export default errorFromValue
