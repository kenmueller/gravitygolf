<script lang="ts">
	export let value = 'The game that combines golf, gravity, and antigravity'
</script>

<svelte:head>
	<meta name="description" content={value} />
	<meta property="og:description" content={value} />
	<meta name="twitter:description" content={value} />
</svelte:head>
