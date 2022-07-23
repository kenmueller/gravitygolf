import type Snapshot from '.'
import type DateLike from './date'

export type ValueType = 'string' | 'number' | 'date'

const get = <Value, DefaultValue>(
	snapshot: Snapshot,
	key: string,
	type: ValueType,
	defaultValue: DefaultValue
) => {
	const value = snapshot.get(key)

	switch (type) {
		case 'date':
			return typeof (value as DateLike)?.toDate === 'function'
				? ((value as DateLike).toDate() as unknown as Value)
				: defaultValue
		case typeof value:
			return value as Value
		default:
			return defaultValue
	}
}

export default get
