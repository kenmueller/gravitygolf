<script lang="ts">
	import favicon from '../../images/icon.png'
	import preview from '../../images/preview.png'

	export let icon = favicon
	export let value = preview
</script>

<svelte:head>
	<link rel="icon" href={icon} />

	<meta property="og:image" content={value} />
	<meta name="twitter:image" content={value} />
</svelte:head>
