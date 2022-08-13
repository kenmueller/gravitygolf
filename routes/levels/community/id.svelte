<script lang="ts">
	import { page } from '$app/stores'

	import Loadable from '../../../components/Loadable.svelte'
	import CommunityLevelPage, {
		load
	} from '../../../components/Level/Page/Community.svelte'
</script>

<Loadable
	load={() => load(fetch, $page.url.searchParams.get('value') ?? '')}
	let:props
>
	<CommunityLevelPage {...props} />
</Loadable>
