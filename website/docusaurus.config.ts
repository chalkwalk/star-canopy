import { themes as prismThemes } from 'prism-react-renderer';
import type { Config } from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

const config: Config = {
  title: 'StarCanopy',
  tagline: 'Procedural space skyboxes, baked on the GPU',
  favicon: 'img/favicon.ico',

  future: {
    v4: true,
  },

  url: 'https://canopy.chalkwalkmusic.com',
  baseUrl: '/',

  organizationName: 'chalkwalk',
  projectName: 'star-canopy',

  onBrokenLinks: 'throw',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.ts',
          routeBasePath: 'docs',
          editUrl: 'https://github.com/chalkwalk/star-canopy/tree/main/website/',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    image: 'img/social-card.png',
    colorMode: {
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: 'StarCanopy',
      // Decorative: the title beside it already carries the name, so an alt
      // text here would only make a screen reader say "StarCanopy" twice.
      logo: {
        alt: '',
        src: 'img/logo.png',
      },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'docsSidebar',
          position: 'left',
          label: 'Manual',
        },
        {
          href: 'https://github.com/chalkwalk/star-canopy',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Manual',
          items: [
            { label: 'Getting started', to: '/docs/getting-started' },
            { label: 'Macros', to: '/docs/macros' },
            { label: 'Outputs', to: '/docs/outputs' },
            { label: 'Gallery', to: '/docs/gallery' },
          ],
        },
        {
          title: 'Project',
          items: [
            { label: 'GitHub', href: 'https://github.com/chalkwalk/star-canopy' },
            {
              label: 'Contributing',
              href: 'https://github.com/chalkwalk/star-canopy/blob/main/CONTRIBUTING.md',
            },
            {
              label: 'Principles',
              href: 'https://github.com/chalkwalk/star-canopy/blob/main/PRINCIPLES.md',
            },
          ],
        },
      ],
      copyright: `StarCanopy is free software under the GPLv3. Copyright &copy; ${new Date().getFullYear()} ChalkWalk.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
      additionalLanguages: ['bash', 'cmake', 'toml'],
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
