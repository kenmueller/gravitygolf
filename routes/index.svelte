<script lang="ts">
	import MetaImage from '../components/Meta/Image.svelte'
	import MetaTitle from '../components/Meta/Title.svelte'
	import MetaDescription from '../components/Meta/Description.svelte'
	import AuthButton from '../components/Auth/Button.svelte'
	import Play from '../images/Play.svelte'
	import Community from '../images/Community.svelte'
	import Edit from '../images/Edit.svelte'
	import GitHub from '../images/GitHub.svelte'
	import Settings from '../images/Settings.svelte'

	const links = [
		{
			href: `/levels/community${MOBILE ? '/mobile' : ''}`,
			icon: Community,
			text: 'Community Levels'
		},
		{
			href: '/levels/editor',
			icon: Edit,
			text: 'Level Editor'
		},
		{
			external: true,
			href: import.meta.env.VITE_GITHUB_URL,
			icon: GitHub,
			text: 'GitHub'
		}
	]
</script>

<MetaImage />
<MetaTitle />
<MetaDescription />

<div class="toolbar">
	<AuthButton />
	<a class="settings" href="/settings" aria-label="Settings">
		<Settings />
	</a>
</div>

<main>
	<h1>Gravity Golf</h1>
	<a class="levels" href="/levels" aria-label="Levels">
		<Play />
	</a>
	{#each links as link (link.href)}
		<a
			class="link"
			href={link.href}
			target={link.external ? '_blank' : undefined}
			rel={link.external ? 'noopener noreferrer' : undefined}
		>
			<svelte:component this={link.icon} />
			<span>{link.text}</span>
		</a>
	{/each}
</main>

<style lang="scss">
	.toolbar {
		display: flex;
		position: absolute;
		top: calc(1.2rem + env(safe-area-inset-top));
		right: calc(1.2rem + env(safe-area-inset-right));
		z-index: 100;
	}

	.settings {
		margin-left: 1rem;
		padding: 0.5rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			width: 2rem;
		}
	}

	main {
		@include scroll.bar;

		position: relative;
		display: flex;
		flex-direction: column;
		align-items: center;
		overflow-y: auto;
		height: 100%;
		padding: 5rem 0 2rem;
		z-index: 0;

		@media (min-height: 38.75rem) {
			justify-content: center;
			padding: 0;
		}
	}

	h1 {
		font-size: 3rem;
		color: white;
	}

	.levels {
		margin: 2rem 0 1rem;
		padding: 1.5rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		@media (min-height: 38.75rem) {
			margin: 4rem 0 1rem;
		}

		&:hover {
			color: white;
		}

		> :global(svg) {
			width: 4rem;
			transform: translateX(0.1rem);
		}
	}

	.link {
		display: flex;
		align-items: center;
		min-width: 16rem;
		margin-top: 1rem;
		padding: 0.5rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			flex-shrink: 0;
			width: 2rem;
		}
	}

	span {
		margin: 0 auto;
	}
</style>
