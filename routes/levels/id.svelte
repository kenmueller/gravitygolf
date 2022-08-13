<script lang="ts">
	import { page } from '$app/stores'

	import Loadable from '../../components/Loadable.svelte'
	import MainLevelPage, { load } from '../../components/Level/Page/Main.svelte'
</script>

<Loadable
	load={() => load($page.url.searchParams.get('value') ?? '')}
	let:props
>
	<MainLevelPage {...props} />
</Loadable>
