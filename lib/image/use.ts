import cache from '../cache'
import ref from '../ref/promise'
import loadImage from './load'

const useImage = (source: string) =>
	cache(`images/${source}`, () => ref(loadImage(source)))

export default useImage
