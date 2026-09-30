import type { SidebarsConfig } from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
  docsSidebar: [
    'intro',
    'getting-started',
    {
      type: 'category',
      label: 'Using StarCanopy',
      collapsed: false,
      items: ['projects', 'macros', 'dials', 'outputs'],
    },
    'gallery',
    'developers',
  ],
};

export default sidebars;
