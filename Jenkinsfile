#!/usr/bin/env groovy
// Jenkins Pipeline for simple-ldapd
// Copyright 2024 SimpleDaemons

pipeline {
    agent any
    
    parameters {
        choice(
            name: 'BUILD_TYPE',
            choices: ['Release', 'Debug'],
            description: 'Build type'
        )
        choice(
            name: 'PLATFORM',
            choices: ['linux', 'macos', 'windows'],
            description: 'Target platform'
        )
        booleanParam(
            name: 'RUN_TESTS',
            defaultValue: true,
            description: 'Run tests'
        )
        booleanParam(
            name: 'RUN_ANALYSIS',
            defaultValue: true,
            description: 'Run static analysis'
        )
        booleanParam(
            name: 'CREATE_PACKAGES',
            defaultValue: false,
            description: 'Create distribution packages'
        )
        booleanParam(
            name: 'DEPLOY',
            defaultValue: false,
            description: 'Deploy artifacts'
        )
    }
    
    environment {
        PROJECT_NAME = 'simple-ldapd'
        VERSION = '${env.BUILD_NUMBER}'
        BUILD_DIR = 'build'
        DIST_DIR = 'dist'
        DOCKER_IMAGE = "simple-ldapd:${VERSION}"
        DOCKER_REGISTRY = 'your-registry.com'
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        
        stage('Build') {
            steps {
                sh '''
                    mkdir -p ${BUILD_DIR}
                    cd ${BUILD_DIR}
                    cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
                    make -j$(nproc)
                '''
            }
        }
        
        stage('Test') {
            when {
                expression { params.RUN_TESTS }
            }
            steps {
                sh '''
                    cd ${BUILD_DIR}
                    make test
                '''
            }
        }
        
        stage('Docker Build') {
            when {
                expression { fileExists("automation/docker/Dockerfile") }
            }
            steps {
                script {
                    def dockerfile = "automation/docker/Dockerfile"
                    if (fileExists(dockerfile)) {
                        sh """
                            docker build -f ${dockerfile} -t ${DOCKER_IMAGE} .
                            docker tag ${DOCKER_IMAGE} ${DOCKER_REGISTRY}/${DOCKER_IMAGE}
                        """
                    } else {
                        echo "No Dockerfile found, skipping Docker build"
                    }
                }
            }
        }
    }
    
    post {
        always {
            archiveArtifacts artifacts: '${BUILD_DIR}/**/*', allowEmptyArchive: true
        }
        success {
            echo "Build succeeded!"
        }
        failure {
            echo "Build failed!"
        }
    }
}

