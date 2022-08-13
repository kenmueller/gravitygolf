<script lang="ts">
	import Loadable from '../../../components/Loadable.svelte'
	import CommunityLevelsPage, {
		load
	} from '../../../components/Level/Page/CommunityLevels.svelte'
</script>

<Loadable load={() => load(fetch)} let:props>
	<CommunityLevelsPage {...props} />
</Loadable>
