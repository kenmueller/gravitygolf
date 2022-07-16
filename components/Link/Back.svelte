<script lang="ts">
	import Back from '../../images/Back.svelte'

	export let href: string
	export let message: string | null = null
	export let focusable = true

	const unload = (event: BeforeUnloadEvent) => {
		if (!message) return

		event.preventDefault()
		return (event.returnValue = message)
	}

	const click = (event: MouseEvent) => {
		if (!message) return
		if (!confirm(message)) event.preventDefault()
	}
</script>

<svelte:window on:beforeunload={message ? unload : undefined} />

<a
	{href}
	tabindex={focusable ? undefined : -1}
	aria-label="Back"
	on:click={message ? click : undefined}
>
	<Back />
	<slot />
</a>

<style lang="scss">
	a {
		display: flex;
		align-items: center;
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			margin: -0.5rem 0 -0.5rem -0.7rem;
			width: 2.5rem;
		}
	}
</style>
