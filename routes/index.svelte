<script lang="ts">
	import MetaImage from '../components/Meta/Image.svelte'
	import MetaTitle from '../components/Meta/Title.svelte'
	import MetaDescription from '../components/Meta/Description.svelte'
	import Play from '../images/Play.svelte'
	import Community from '../images/Community.svelte'
	import Edit from '../images/Edit.svelte'
	import GitHub from '../images/GitHub.svelte'
	import Settings from '../images/Settings.svelte'

	const links = [
		{
			href: '/levels/community',
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
	<a class="settings" href="/settings" aria-label="Settings">
		<Settings />
	</a>
</main>

<style lang="scss">
	main {
		position: relative;
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		overflow: hidden;
		height: 100%;
	}

	h1 {
		font-size: 3rem;
		color: white;
	}

	.levels {
		margin: 4rem 0 1rem;
		padding: 1.5rem;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

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
			width: 2rem;
		}
	}

	span {
		margin: 0 auto;
	}

	.settings {
		position: absolute;
		top: calc(1.2rem + env(safe-area-inset-top));
		right: calc(1.2rem + env(safe-area-inset-right));
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
</style>
