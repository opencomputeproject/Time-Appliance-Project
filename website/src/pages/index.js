/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @format
 */

import React from 'react';
import clsx from 'clsx';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import useBaseUrl from '@docusaurus/useBaseUrl';
import styles from './styles.module.css';

const features = [
  {
    title: 'Open Time Server',
    // imageUrl: 'img/timecard-logo.svg',
    path: 'docs/open-time-server',
    description: (
      <>
        The Open Time Server (OTS) is an Open, Scalable and Validated reference
        architecture that can be deployed in Data Centers or in an edge
        environments.
      </>
    ),
  },
  {
    title: 'Time Card',
    path: 'docs/time-card/introduction',
    // imageUrl: 'img/timecard-logo.svg',
    description: (
      <>
        The Time Card is a critical part of a PTP enabled network providing
        accurate time via GNSS while maintaining accuracy in case of GNSS
        failure via a high stability (and holdover) oscillator.
      </>
    ),
  },
  {
    title: 'Datacenter PTP Profile',
    path: 'docs/dc-ptp-profile',
    // imageUrl: 'img/timecard-logo.svg',
    description: (
      <>
        A PTP profile for time-sensitive applications. This profile defines a
        set of requirements for implementing, deploying and operating timing
        appliances within a data center.
      </>
    ),
  },
];

function Feature({imageUrl, title, description, path}) {
  const imgUrl = useBaseUrl(imageUrl);
  return (
    <div className={clsx('col', styles.feature)}>
      {imgUrl && (
        <div className="text--center">
          <img className={styles.featureImage} src={imgUrl} alt={title} />
        </div>
      )}
      <h3>{title}</h3>
      <p>{description}</p>
      <div className={styles.buttons}>
        <Link
          className={clsx(
            'button button--outline button--secondary button--lg',
            styles.getStarted,
          )}
          to={useBaseUrl(path)}>
          View More
        </Link>
      </div>
    </div>
  );
}

export default function Home() {
  const context = useDocusaurusContext();
  const {siteConfig = {}} = context;
  return (
    <Layout
      title={`${siteConfig.title}`}
      description="Develop an end-to-end hypothetical reference model, 
       network architectures, performance objectives and the methods 
       to distribute, operate, monitor time synchronization within
       data center and much more.">
      <header className={clsx('hero hero--primary', styles.heroBanner)}>
        <div className="container">
          <h1 className="hero__title">{siteConfig.title}</h1>
          <p className="hero__subtitle">{siteConfig.tagline}</p>
          <div className={styles.buttons}>
            <Link
              className={clsx(
                'button button--outline button--secondary button--lg',
                styles.getStarted,
              )}
              to={useBaseUrl('docs/intro')}>
              Get Started
            </Link>
          </div>
        </div>
      </header>
      <main>
        {features && features.length > 0 && (
          <section className={styles.features}>
            <div className="container">
              <div className="row">
                {features.map(({title, imageUrl, description, path}) => (
                  <Feature
                    key={title}
                    title={title}
                    imageUrl={imageUrl}
                    description={description}
                    path={path}
                  />
                ))}
              </div>
            </div>
          </section>
        )}
      </main>
    </Layout>
  );
}
