import type Snapshot from '.'

const exists = (snapshot: Snapshot) =>
	typeof snapshot.exists === 'function' ? snapshot.exists() : snapshot.exists

export default exists
