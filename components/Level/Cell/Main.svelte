<script lang="ts">
	import stars from '$lib/level/stars'
	import Base from './Base.svelte'

	export let id: number
</script>

<Base
	href="/levels/{MOBILE ? 'id?value=' : ''}{encodeURIComponent(id)}"
	disabled={id - 1 > ($stars?.length ?? 0)}
	stars={$stars?.[id - 1] ?? null}
>
	Level {id}
</Base>
