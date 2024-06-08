# KMS

The KMS uses 2 containers, one for the app KMS itself and another for the database.

## Prerequisites

- Docker

## Setup and Running

1. Clone the repository:

```bash
git clone git@github.com:PatriciaCardoso2002/KMS.git
```

2. Navigate to the project directory:

```bash
cd <project_directory>
```

3. Build and start the services:

```bash
docker compose up --build
```
This command builds the Docker images for the services defined in the compose.yml file, creates the containers, and starts the services.

You need to insert the database's IP Address in the KMS input parameters, after running it for the first time. You can see it in a different terminal with :

```bash
docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' kms-kms_db-1
```

Stop the containers using `CTRL+C`. After that, remove them using:

```bash
docker container prune -f
```

You will need to build and start them again:

```bash
docker compose up --build
```

This time the application should run flawlessly. If not, stop the containers and repeat the previous command.

## Stopping the services

To stop the services, use the down command:

```bash
docker compose down
```

This command stops and removes the containers, networks, and volumes defined in the compose.yml file.

## Running the services again

To run the services again, use the up command:

```bash
docker compose up
```

If you want to rebuild the images before running the services, you can use the --build option:

```bash
docker compose up --build
```

## APP and KPS

Both the APP and KPS are present in this repository.

To run them you need to navigate to each respective folder and compile.

```bash
cd <APP or KPS>
make
./executable_file
```
