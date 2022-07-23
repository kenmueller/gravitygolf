import { getAuth, signOut as _signOut } from 'firebase/auth'

import app from '$lib/firebase'

const auth = getAuth(app)

const signOut = () => _signOut(auth)

export default signOut
